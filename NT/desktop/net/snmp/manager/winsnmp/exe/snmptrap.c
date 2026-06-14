/* snmptrap.c - SNMP Trap Service
 * Original Microsoft code modified by ACE*COMM
 */

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wsipx.h>
#include <process.h>

#ifdef DBG
#include <stdio.h>
#include <time.h>
#endif

//--------------------------- PRIVATE DEFINITIONS -----------------------------
#define SNMPMGRTRAPPIPE "\\\\.\\PIPE\\MGMTAPI"
#define MAX_OUT_BUFS    16
#define TRAPBUFSIZE     4096
#define IP_TRAP_PORT    162
#define IPX_TRAP_PORT   36880
#define SNMPTRAP_WAIT_HINT 20000

#define MAX_UDP_SIZE        (65535-8)
#define MAX_FIONREAD_UDP_SIZE 8192
#define FOUR_K_BUF_SIZE     4096
#define MAXUDPLEN_BUFFER_TIME (2*60*1000)

// Linked list macros
#define ll_init(head) (head)->next = (head)->prev = (head)
#define ll_empt(head) (((head)->next) == (head))
#define ll_next(item, head) (((item)->next == (head)) ? NULL : (item)->next)
#define ll_prev(item) ((item)->prev)
#define ll_adde(item, head) \
    do { \
        ll_node *pred = (head)->prev; \
        (item)->next = (head); \
        (item)->prev = pred; \
        pred->next = (item); \
        (head)->prev = (item); \
    } while(0)
#define ll_rmv(item) \
    do { \
        ll_node *pred = (item)->prev; \
        ll_node *succ = (item)->next; \
        pred->next = succ; \
        succ->prev = pred; \
    } while(0)

//--------------------------- TYPE DEFINITIONS -------------------------------
typedef struct ll_s {
    struct ll_s *next;
    struct ll_s *prev;
} ll_node;

typedef struct {
    ll_node  links;
    HANDLE   hPipe;
} svrPipeListEntry;

typedef struct {
    SOCKADDR Addr;
    int      AddrLen;
    UINT     TrapBufSz;
    char     TrapBuf[TRAPBUFSIZE];
} SNMP_TRAP, *PSNMP_TRAP;

typedef struct {
    SOCKET s;
    OVERLAPPED ol;
} TRAP_THRD_CONTEXT, *PTRAP_THRD_CONTEXT;

//--------------------------- GLOBAL VARIABLES -------------------------------
HANDLE hExitEvent = NULL;
LPCTSTR svcName = "SNMPTRAP";
SERVICE_STATUS_HANDLE hService = NULL;
SERVICE_STATUS status = {
    SERVICE_WIN32, 
    SERVICE_STOPPED, 
    SERVICE_ACCEPT_STOP, 
    NO_ERROR, 
    0, 
    0, 
    0
};

SOCKET ipSock = INVALID_SOCKET;
SOCKET ip6Sock = INVALID_SOCKET;
SOCKET ipxSock = INVALID_SOCKET;
HANDLE ipThread = NULL;
HANDLE ip6Thread = NULL;
HANDLE ipxThread = NULL;
CRITICAL_SECTION cs_PIPELIST;
ll_node *pSvrPipeListHead = NULL;

OVERLAPPED g_ol;
TRAP_THRD_CONTEXT g_ipThreadContext;
TRAP_THRD_CONTEXT g_ip6ThreadContext;
TRAP_THRD_CONTEXT g_ipxThreadContext;

//--------------------------- FUNCTION PROTOTYPES ----------------------------
DWORD WINAPI svrTrapThread(LPVOID threadParam);
DWORD WINAPI svrPipeThread(LPVOID threadParam);
VOID WINAPI svcHandlerFunction(DWORD dwControl);
VOID WINAPI svcMainFunction(DWORD dwNumServicesArgs, LPSTR *lpServiceArgVectors);
void FreeSvrPipeEntryList(ll_node* head);
PACL AllocGenericACL(void);

#ifdef DBG
VOID WINAPI SnmpTrapDbgPrint(LPSTR szFormat, ...);
#define SNMPTRAPDBG(_x_) SnmpTrapDbgPrint _x_
#else
#define SNMPTRAPDBG(_x_)
#endif

//--------------------------- IMPLEMENTATION ---------------------------------
PACL AllocGenericACL(void)
{
    PACL pAcl;
    PSID pSidAdmins, pSidUsers, pSidLocalService;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    DWORD aclLength;

    pSidAdmins = pSidUsers = pSidLocalService = NULL;

    if (!AllocateAndInitializeSid(&ntAuthority, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &pSidAdmins) ||
        !AllocateAndInitializeSid(&ntAuthority, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_USERS,
                                   0, 0, 0, 0, 0, 0,
                                   &pSidUsers) ||
        !AllocateAndInitializeSid(&ntAuthority, 1,
                                   SECURITY_LOCAL_SERVICE_RID,
                                   0,
                                   0, 0, 0, 0, 0, 0,
                                   &pSidLocalService))
    {
        if (pSidAdmins) FreeSid(pSidAdmins);
        if (pSidUsers) FreeSid(pSidUsers);
        return NULL;
    }

    aclLength = sizeof(ACL) +
                sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                GetLengthSid(pSidAdmins) +
                sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                GetLengthSid(pSidUsers) +
                sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                GetLengthSid(pSidLocalService);

    pAcl = (PACL)GlobalAlloc(GPTR, aclLength);
    if (pAcl != NULL)
    {
        if (!InitializeAcl(pAcl, aclLength, ACL_REVISION) ||
            !AddAccessAllowedAce(pAcl, ACL_REVISION,
                                 GENERIC_READ | GENERIC_WRITE,
                                 pSidLocalService) ||
            !AddAccessAllowedAce(pAcl, ACL_REVISION,
                                 GENERIC_READ | GENERIC_WRITE,
                                 pSidAdmins) ||
            !AddAccessAllowedAce(pAcl, ACL_REVISION,
                                 (GENERIC_READ | (FILE_GENERIC_WRITE & ~FILE_CREATE_PIPE_INSTANCE)),
                                 pSidUsers))
        {
            GlobalFree(pAcl);
            pAcl = NULL;
        }
    }

    FreeSid(pSidAdmins);
    FreeSid(pSidUsers);
    FreeSid(pSidLocalService);

    return pAcl;
}

void FreeSvrPipeEntryList(ll_node* head)
{
    ll_node *current, *hold;

    if (head == NULL)
        return;
    
    current = head;
    while ((current = ll_next(current, head)) != NULL)
    {
        DisconnectNamedPipe(((svrPipeListEntry *)current)->hPipe);
        CloseHandle(((svrPipeListEntry *)current)->hPipe);
        
        hold = ll_prev(current);
        ll_rmv(current);
        GlobalFree(current);
        current = hold;
    }
    GlobalFree(head);
}

int __cdecl main(void)
{
    BOOL fOk;
    SERVICE_TABLE_ENTRY svcStartTable[2] =
    {
        {(LPTSTR)svcName, svcMainFunction},
        {NULL, NULL}
    };
    
    
    HeapSetInformation(NULL, HeapCompatibilityInformation, NULL, 0);
    
    hExitEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (hExitEvent == NULL)
        exit(1);
    
    memset(&g_ol, 0, sizeof(g_ol));
    g_ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (g_ol.hEvent == NULL)
    {
        CloseHandle(hExitEvent);
        exit(1);
    }
    
	fOk = StartServiceCtrlDispatcher(svcStartTable);
    if (!StartServiceCtrlDispatcher(svcStartTable))
    {
        CloseHandle(hExitEvent);
        CloseHandle(g_ol.hEvent);
        exit(1);
    }
    
    CloseHandle(hExitEvent);
    CloseHandle(g_ol.hEvent);
    return fOk;
}

VOID WINAPI svcHandlerFunction(DWORD dwControl)
{
    if (dwControl == SERVICE_CONTROL_STOP)
    {
        status.dwCheckPoint++;
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwWaitHint = SNMPTRAP_WAIT_HINT;
        
        if (!SetServiceStatus(hService, &status))
            exit(1);
        
        if (!SetEvent(hExitEvent))
        {
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 1;
            SetServiceStatus(hService, &status);
            exit(1);
        }
    }
    else
    {
        if (status.dwCurrentState == SERVICE_STOP_PENDING ||
            status.dwCurrentState == SERVICE_START_PENDING)
        {
            status.dwCheckPoint++;
        }
        
        if (!SetServiceStatus(hService, &status))
            exit(1);
    }
}

VOID WINAPI svcMainFunction(DWORD dwNumServicesArgs, LPSTR *lpServiceArgVectors)
{
    WSADATA wsaData;
    HANDLE hPipeThread = NULL;
    DWORD threadId;
    int result;
    struct addrinfo hints, *addrInfo = NULL;
    struct sockaddr_ipx ipxAddr;
    char snmpTrapPort[] = "162";
    char snmpTrapService[] = "snmp-trap";
    LPWSTR cmdLine;
    DWORD pipeThreadId;

    cmdLine = GetCommandLineW();
    RegisterApplicationRestart(cmdLine, 0);
    
    hService = RegisterServiceCtrlHandlerA("SNMPTRAP", svcHandlerFunction);
    if (hService == NULL)
        goto exit_failure;

    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwWaitHint = SNMPTRAP_WAIT_HINT;
    if (!SetServiceStatus(hService, &status))
        goto exit_failure;

    InitializeCriticalSection(&cs_PIPELIST);

    memset(&g_ipThreadContext.ol, 0, sizeof(g_ipThreadContext.ol));
    memset(&g_ip6ThreadContext.ol, 0, sizeof(g_ip6ThreadContext.ol));
    memset(&g_ipxThreadContext.ol, 0, sizeof(g_ipxThreadContext.ol));

    result = WSAStartup(MAKEWORD(1, 1), &wsaData);
    if (result != 0)
        goto close_out;

    pSvrPipeListHead = (ll_node *)GlobalAlloc(GPTR, sizeof(ll_node));
    if (pSvrPipeListHead == NULL)
        goto close_out;
    
    ll_init(pSvrPipeListHead);
    
    hPipeThread = (HANDLE)_beginthreadex(NULL, 0, svrPipeThread, NULL, 0, &pipeThreadId);
    if (hPipeThread == 0)
        goto close_out;

    // IPv4 Socket
    ipSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ipSock != INVALID_SOCKET)
    {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        
        result = getaddrinfo(NULL, snmpTrapService, &hints, &addrInfo);
        if (result != 0)
            result = getaddrinfo(NULL, snmpTrapPort, &hints, &addrInfo);
        
        if (result == 0)
        {
            result = bind(ipSock, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);
            if (result != SOCKET_ERROR)
            {
                g_ipThreadContext.s = ipSock;
                g_ipThreadContext.ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
                if (g_ipThreadContext.ol.hEvent != NULL)
                {
                    ipThread = (HANDLE)_beginthreadex(NULL, 0, svrTrapThread,
                                                       &g_ipThreadContext, 0, &threadId);
                }
            }
            freeaddrinfo(addrInfo);
            addrInfo = NULL;
        }
    }

    // IPv6 Socket
    ip6Sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (ip6Sock != INVALID_SOCKET)
    {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        
        result = getaddrinfo(NULL, snmpTrapService, &hints, &addrInfo);
        if (result != 0)
            result = getaddrinfo(NULL, snmpTrapPort, &hints, &addrInfo);
        
        if (result == 0)
        {
            result = bind(ip6Sock, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);
            if (result != SOCKET_ERROR)
            {
                g_ip6ThreadContext.s = ip6Sock;
                g_ip6ThreadContext.ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
                if (g_ip6ThreadContext.ol.hEvent != NULL)
                {
                    ip6Thread = (HANDLE)_beginthreadex(NULL, 0, svrTrapThread,
                                                        &g_ip6ThreadContext, 0, &threadId);
                }
            }
            freeaddrinfo(addrInfo);
            addrInfo = NULL;
        }
    }

    // IPX Socket
    ipxSock = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
    if (ipxSock != INVALID_SOCKET)
    {
        memset(&ipxAddr, 0, sizeof(ipxAddr));
        ipxAddr.sa_family = AF_IPX;
        ipxAddr.sa_socket = htons(IPX_TRAP_PORT);
        
        result = bind(ipxSock, (struct sockaddr *)&ipxAddr, sizeof(ipxAddr));
        if (result != SOCKET_ERROR)
        {
            g_ipxThreadContext.s = ipxSock;
            g_ipxThreadContext.ol.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            if (g_ipxThreadContext.ol.hEvent != NULL)
            {
                ipxThread = (HANDLE)_beginthreadex(NULL, 0, svrTrapThread,
                                                    &g_ipxThreadContext, 0, &threadId);
            }
        }
    }

    status.dwCurrentState = SERVICE_RUNNING;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    if (!SetServiceStatus(hService, &status))
        goto close_out;

    WaitForSingleObject(hExitEvent, INFINITE);

close_out:
    SetEvent(hExitEvent);

    status.dwCurrentState = SERVICE_STOP_PENDING;
    status.dwCheckPoint++;
    SetServiceStatus(hService, &status);

    // Cleanup pipe thread
    if (hPipeThread != NULL)
    {
        SetEvent(g_ol.hEvent);
        WaitForSingleObject(hPipeThread, INFINITE);
        status.dwCheckPoint++;
        SetServiceStatus(hService, &status);
        CloseHandle(hPipeThread);
    }

    // Cleanup IPv4
    if (ipThread != NULL)
    {
        SetEvent(g_ipThreadContext.ol.hEvent);
        WaitForSingleObject(ipThread, INFINITE);
        status.dwCheckPoint++;
        SetServiceStatus(hService, &status);
        CloseHandle(ipThread);
    }
    if (ipSock != INVALID_SOCKET)
        closesocket(ipSock);
    if (g_ipThreadContext.ol.hEvent != NULL)
        CloseHandle(g_ipThreadContext.ol.hEvent);

    // Cleanup IPv6
    if (ip6Thread != NULL)
    {
        SetEvent(g_ip6ThreadContext.ol.hEvent);
        WaitForSingleObject(ip6Thread, INFINITE);
        status.dwCheckPoint++;
        SetServiceStatus(hService, &status);
        CloseHandle(ip6Thread);
    }
    if (ip6Sock != INVALID_SOCKET)
        closesocket(ip6Sock);
    if (g_ip6ThreadContext.ol.hEvent != NULL)
        CloseHandle(g_ip6ThreadContext.ol.hEvent);

    // Cleanup IPX
    if (ipxThread != NULL)
    {
        SetEvent(g_ipxThreadContext.ol.hEvent);
        WaitForSingleObject(ipxThread, INFINITE);
        status.dwCheckPoint++;
        SetServiceStatus(hService, &status);
        CloseHandle(ipxThread);
    }
    if (ipxSock != INVALID_SOCKET)
        closesocket(ipxSock);
    if (g_ipxThreadContext.ol.hEvent != NULL)
        CloseHandle(g_ipxThreadContext.ol.hEvent);

    EnterCriticalSection(&cs_PIPELIST);
    if (pSvrPipeListHead != NULL)
    {
        FreeSvrPipeEntryList(pSvrPipeListHead);
        pSvrPipeListHead = NULL;
    }
    LeaveCriticalSection(&cs_PIPELIST);

    DeleteCriticalSection(&cs_PIPELIST);
    WSACleanup();

    status.dwCurrentState = SERVICE_STOPPED;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    if (!SetServiceStatus(hService, &status))
        exit(1);
    
    return;

exit_failure:
    exit(1);
}

DWORD WINAPI svrPipeThread(LPVOID threadParam)
{
    SECURITY_ATTRIBUTES secAttrib;
    SECURITY_DESCRIPTOR secDesc;
    PACL pAcl;
    DWORD bytesRead;
    HANDLE hPipe;
    svrPipeListEntry *item;
    BOOL connectSuccess;
    ll_node *current, *hold;
    DWORD lastError;

    InitializeSecurityDescriptor(&secDesc, SECURITY_DESCRIPTOR_REVISION);
    
    pAcl = AllocGenericACL();
    if (pAcl == NULL)
        return 0;
    
    if (!SetSecurityDescriptorDacl(&secDesc, TRUE, pAcl, FALSE))
    {
        GlobalFree(pAcl);
        return 0;
    }

    secAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttrib.lpSecurityDescriptor = &secDesc;
    secAttrib.bInheritHandle = TRUE;

    while (TRUE)
    {
        if (WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0)
            break;

        hPipe = CreateNamedPipeA(SNMPMGRTRAPPIPE,
                                  PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                  PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
                                  0xFF,
                                  0x10880, 0x1088, 0,
                                  &secAttrib);

        if (hPipe == INVALID_HANDLE_VALUE)
            break;

        connectSuccess = ConnectNamedPipe(hPipe, &g_ol);
        if (!connectSuccess)
        {
            lastError = GetLastError();
            if (lastError == ERROR_IO_PENDING)
            {
                GetOverlappedResult(hPipe, &g_ol, &bytesRead, TRUE);
                
                if (WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0)
                {
                    CloseHandle(hPipe);
                    break;
                }
                ResetEvent(g_ol.hEvent);
            }
            else if (lastError != ERROR_PIPE_CONNECTED)
            {
                CloseHandle(hPipe);
                continue;
            }
        }

        item = (svrPipeListEntry *)GlobalAlloc(GPTR, sizeof(svrPipeListEntry));
        if (item == NULL)
        {
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            break;
        }

        item->hPipe = hPipe;

        EnterCriticalSection(&cs_PIPELIST);
        ll_adde((ll_node *)item, pSvrPipeListHead);
        
        current = pSvrPipeListHead;
        while ((current = ll_next(current, pSvrPipeListHead)) != NULL)
        {
            ConnectNamedPipe(((svrPipeListEntry *)current)->hPipe, NULL);
            lastError = GetLastError();

            if (lastError != ERROR_PIPE_CONNECTED)
            {
                DisconnectNamedPipe(((svrPipeListEntry *)current)->hPipe);
                CloseHandle(((svrPipeListEntry *)current)->hPipe);
                
                hold = ll_prev(current);
                ll_rmv(current);
                GlobalFree(current);
                current = hold;
            }
        }
        LeaveCriticalSection(&cs_PIPELIST);
    }

    GlobalFree(pAcl);
    return 0;
}

DWORD WINAPI svrTrapThread(LPVOID threadParam)
{
    PSNMP_TRAP pRecvTrap = NULL;
    fd_set readfds;
    PTRAP_THRD_CONTEXT pThreadContext = (PTRAP_THRD_CONTEXT)threadParam;
    SOCKET sock;
    int recvLen;
    DWORD lastAllocatedSize = 0;
    DWORD lastBigBufferTime = 0;
    BOOL timeoutForBigBuffer = FALSE;
    struct timeval selectTimeout;
    ULONG pendingSize = 0;
    DWORD lastError;
    int selectResult;
    ll_node *item, *hold;
    DWORD bytesWritten;

    if (pThreadContext == NULL)
        return 0;

    sock = pThreadContext->s;
    lastBigBufferTime = GetTickCount();

    selectTimeout.tv_sec = 5;
    selectTimeout.tv_usec = 0;

    while (TRUE)
    {
        if (WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0)
            break;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        selectResult = select(0, &readfds, NULL, NULL, &selectTimeout);
        if (selectResult == 0)
            continue;
        else if (selectResult == SOCKET_ERROR)
            break;

        if (!FD_ISSET(sock, &readfds))
            continue;

        if (ioctlsocket(sock, FIONREAD, &pendingSize) != 0)
        {
            WSAGetLastError();
            continue;
        }

        if (pendingSize >= MAX_FIONREAD_UDP_SIZE)
        {
            lastBigBufferTime = GetTickCount();

            if (pRecvTrap == NULL || lastAllocatedSize < MAX_UDP_SIZE)
            {
                if (pRecvTrap != NULL)
                {
                    GlobalFree(pRecvTrap);
                    pRecvTrap = NULL;
                    lastAllocatedSize = 0;
                }

                pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, sizeof(SNMP_TRAP) - TRAPBUFSIZE + MAX_UDP_SIZE);
                if (pRecvTrap == NULL)
                {
                    lastAllocatedSize = 0;
                    break;
                }
                lastAllocatedSize = MAX_UDP_SIZE;
            }
        }
        else
        {
            timeoutForBigBuffer = FALSE;
            
            if (lastAllocatedSize == MAX_UDP_SIZE)
            {
                DWORD currentTime = GetTickCount();
                if (currentTime < lastBigBufferTime ||
                    (currentTime - lastBigBufferTime) > MAXUDPLEN_BUFFER_TIME)
                {
                    timeoutForBigBuffer = TRUE;
                }
            }

            if (pRecvTrap == NULL || timeoutForBigBuffer || lastAllocatedSize < pendingSize)
            {
                if (pRecvTrap != NULL)
                {
                    GlobalFree(pRecvTrap);
                    pRecvTrap = NULL;
                    lastAllocatedSize = 0;
                }

                if (pendingSize <= FOUR_K_BUF_SIZE)
                {
                    pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, sizeof(SNMP_TRAP) - TRAPBUFSIZE + FOUR_K_BUF_SIZE);
                    lastAllocatedSize = FOUR_K_BUF_SIZE;
                }
                else
                {
                    pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, sizeof(SNMP_TRAP) - TRAPBUFSIZE + pendingSize);
                    lastAllocatedSize = pendingSize;
                }

                if (pRecvTrap == NULL)
                {
                    lastAllocatedSize = 0;
                    break;
                }
            }
        }

        pRecvTrap->TrapBufSz = lastAllocatedSize;
        pRecvTrap->AddrLen = sizeof(pRecvTrap->Addr);

        recvLen = recvfrom(sock, pRecvTrap->TrapBuf, pRecvTrap->TrapBufSz, 0,
                           &(pRecvTrap->Addr), &(pRecvTrap->AddrLen));

        if (recvLen == SOCKET_ERROR)
        {
            WSAGetLastError();
            continue;
        }

        EnterCriticalSection(&cs_PIPELIST);
        pRecvTrap->TrapBufSz = recvLen;
        recvLen += sizeof(SNMP_TRAP) - sizeof(pRecvTrap->TrapBuf);

        if (!ll_empt(pSvrPipeListHead))
        {
            item = pSvrPipeListHead;
            while ((item = ll_next(item, pSvrPipeListHead)) != NULL)
            {
                if (WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0)
                {
                    LeaveCriticalSection(&cs_PIPELIST);
                    goto cleanup;
                }

                if (!WriteFile(((svrPipeListEntry *)item)->hPipe, (LPBYTE)pRecvTrap,
                               recvLen, &bytesWritten, &pThreadContext->ol))
                {
                    lastError = GetLastError();
                    if (lastError == ERROR_IO_PENDING)
                    {
                        GetOverlappedResult(((svrPipeListEntry *)item)->hPipe,
                                           &pThreadContext->ol, &bytesWritten, TRUE);
                        
                        if (WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0)
                        {
                            LeaveCriticalSection(&cs_PIPELIST);
                            goto cleanup;
                        }
                        ResetEvent(pThreadContext->ol.hEvent);
                    }
                    else
                    {
                        DisconnectNamedPipe(((svrPipeListEntry *)item)->hPipe);
                        CloseHandle(((svrPipeListEntry *)item)->hPipe);
                        hold = ll_prev(item);
                        ll_rmv(item);
                        GlobalFree(item);
                        item = hold;
                    }
                }
            }
        }
        LeaveCriticalSection(&cs_PIPELIST);
    }

cleanup:
    if (pRecvTrap != NULL)
        GlobalFree(pRecvTrap);

    return 0;
}

#ifdef DBG
#define MAX_LOG_ENTRY_LEN 512
VOID WINAPI SnmpTrapDbgPrint(LPSTR szFormat, ...)
{
    va_list arglist;
    char logEntry[4 * MAX_LOG_ENTRY_LEN];
    time_t now;

    va_start(arglist, szFormat);
    time(&now);
    strftime(logEntry, MAX_LOG_ENTRY_LEN, "%H:%M:%S :", localtime(&now));
    vsprintf(logEntry + strlen(logEntry), szFormat, arglist);
    OutputDebugStringA(logEntry);
    va_end(arglist);
}
#endif

