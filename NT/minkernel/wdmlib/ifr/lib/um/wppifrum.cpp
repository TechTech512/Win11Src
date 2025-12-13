#pragma warning (disable:4005)

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <strsafe.h>

// Structure definitions
typedef struct _WPP_PROJECT_CONTROL_BLOCK {
    struct {
        void* AutoLogContext;
        USHORT AutoLogVerboseEnabled;
        USHORT AutoLogAttachToMiniDump;
    } Control;
    struct _WPP_LOG_BUFFER* pLogBuffer;  // 0x28
} WPP_PROJECT_CONTROL_BLOCK, *PWPP_PROJECT_CONTROL_BLOCK;

typedef struct _WPP_LOG_BUFFER {
    ULONGLONG StartTimestamp;           // 0x14
    struct _WPP_RECORDER_CONTROL_BLOCK* pControlBlock;  // 0x38
} WPP_LOG_BUFFER, *PWPP_LOG_BUFFER;

typedef struct _WPP_RECORDER_CONTROL_BLOCK {
    // 0x00-0x27: Unknown fields
    struct _WPP_LOG_BUFFER* pLogBuffer;  // 0x28
    // 0x30-0x5F: Unknown fields
    ULONG BufferHeaderSize;             // 0x60
    // Rest of structure...
} WPP_RECORDER_CONTROL_BLOCK, *PWPP_RECORDER_CONTROL_BLOCK;

int __cdecl GetWppAutoLogRegistrySettings(void* param_1, DWORD* param_2, DWORD* param_3, DWORD* param_4, DWORD* param_5)
{
    HKEY hKeyServices = NULL;
    HKEY hKeyParameters = NULL;
    DWORD dwValueType = 0;
    DWORD dwValueSize = sizeof(DWORD);
    int iResult = 1;
    
    *param_2 = 0;
    *param_3 = 0;
    *param_4 = 0x1000;
    *((DWORD*)param_1) = 1;
    *((DWORD*)((BYTE*)param_1 + 4)) = 0;
    *param_5 = 0;
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services", 0, 
                      KEY_READ | KEY_WOW64_64KEY, &hKeyServices) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hKeyServices, L"Parameters", 0, KEY_READ | KEY_WOW64_64KEY, &hKeyParameters) == ERROR_SUCCESS)
        {
            if (RegQueryValueExW(hKeyParameters, L"VerboseOn", NULL, &dwValueType, 
                                 (LPBYTE)param_2, &dwValueSize) != ERROR_SUCCESS || dwValueType != REG_DWORD)
            {
                iResult = 1;
            }
            
            dwValueSize = sizeof(DWORD);
            if (RegQueryValueExW(hKeyParameters, L"ForceLogsInMiniDump", NULL, &dwValueType, 
                                 (LPBYTE)param_1, &dwValueSize) != ERROR_SUCCESS || dwValueType != REG_DWORD)
            {
                iResult = 1;
            }
            
            dwValueSize = sizeof(DWORD);
            if (RegQueryValueExW(hKeyParameters, L"LogPages", NULL, &dwValueType, 
                                 (LPBYTE)param_4, &dwValueSize) == ERROR_SUCCESS && dwValueType == REG_DWORD)
            {
                *param_4 = *param_4 << 12;
            }
            else
            {
                iResult = 1;
            }
            
            dwValueSize = sizeof(DWORD);
            if (RegQueryValueExW(hKeyParameters, L"PerDriverDisableIFR", NULL, &dwValueType, 
                                 (LPBYTE)param_3, &dwValueSize) != ERROR_SUCCESS || dwValueType != REG_DWORD)
            {
                iResult = 1;
            }
            
            if (*param_4 > 0x10000 || *param_4 < 0x1000)
            {
                *param_4 = (*param_4 > 0x10000) ? 0x10000 : 0x1000;
                iResult = 1;
            }
            
            RegCloseKey(hKeyParameters);
            RegCloseKey(hKeyServices);
            return iResult;
        }
        RegCloseKey(hKeyServices);
    }
    
    return 1;
}

void* __cdecl WppAutoLogGetDefaultHandle(void* param_1)
{
    void* pResult = NULL;
    
    if (param_1 != NULL)
    {
        pResult = *(void**)((BYTE*)param_1 + 0x24);
        if (pResult != NULL)
        {
            pResult = *(void**)((BYTE*)pResult + 0x24);
        }
    }
    
    return pResult;
}

void __cdecl WppAutoLogStart(WPP_PROJECT_CONTROL_BLOCK* param_1, void* param_2, void* param_3)
{
    int* pDriverExtension = NULL;
    DWORD dwVerboseEnabled = 0;
    DWORD dwBufferSize = 0;
    int iAllocatedBuffer = 0;
    DWORD dwForceLogsValue = 0;
    DWORD dwUnused1 = 0;
    DWORD dwUnused2 = 0;
    DWORD dwUnused3 = 0;
    
    if (param_2 != NULL)
    {
        pDriverExtension = *(int**)((BYTE*)param_2 + 0x14);
    }
    
    if (pDriverExtension == NULL)
    {
        param_1->Control.AutoLogContext = NULL;
        return;
    }
    
    WCHAR* pRegistryPath = *(WCHAR**)((BYTE*)param_3 + 4);
    if (pRegistryPath != NULL)
    {
        const WCHAR* pLastBackslash = wcsrchr(pRegistryPath, L'\\');
        if (pLastBackslash != NULL)
        {
            const WCHAR* pDriverName = pLastBackslash + 1;
            
            GetWppAutoLogRegistrySettings(&dwForceLogsValue, &dwVerboseEnabled, &dwUnused1, &dwUnused2, &dwUnused3);
            
            param_1->Control.AutoLogVerboseEnabled = (USHORT)dwVerboseEnabled;
            param_1->Control.AutoLogAttachToMiniDump = (USHORT)dwForceLogsValue;
            
            typedef int (__cdecl* DRIVER_FUNC_PTR)(int*, WCHAR*, DWORD, DWORD, DWORD, int*, DWORD*);
            DRIVER_FUNC_PTR pDriverFunc = (DRIVER_FUNC_PTR)(*(DWORD_PTR*)(*pDriverExtension + 0x100));
            
            if (pDriverFunc != NULL)
            {
                int iFuncResult = pDriverFunc(pDriverExtension, (WCHAR*)pDriverName, dwForceLogsValue >> 12, 
                                              1, dwForceLogsValue, &iAllocatedBuffer, &dwBufferSize);
                
                if ((iAllocatedBuffer == 0) || (iFuncResult < 0) || (dwBufferSize < 0x7C))
                {
                    param_1->Control.AutoLogContext = NULL;
                }
                else
                {
                    BYTE* pBuffer = (BYTE*)iAllocatedBuffer;
                    
                    *(DWORD*)(pBuffer + 0x34) = 1;
                    *(int*)(pBuffer + 0x30) = (int)param_2 + 0x44;
                    *(BYTE*)(pBuffer + 0xC) = 0;
                    *(DWORD*)(pBuffer + 0x2C) = dwVerboseEnabled;
                    *(int*)(pBuffer + 0x5C) = iAllocatedBuffer;
                    *(DWORD*)(pBuffer + 0x78) = dwBufferSize - 0x7C;
                    pBuffer[0x38] = 0;
                    *(DWORD*)(pBuffer + 0x50) = 0;
                    
                    StringCchCopyA((char*)(pBuffer + 0x38), 16, "WudfDriverLog");
                    
                    *(int*)(pBuffer + 0x6C) = iAllocatedBuffer + 0x7C;
                    *(DWORD*)(pBuffer + 0x70) = *(DWORD*)(pBuffer + 0x78);
                    *(DWORD*)(pBuffer + 0x74) = 0;
                    *(char**)(pBuffer + 0x28) = (char*)(pBuffer + 0x38);
                    *(int*)(pBuffer + 0x54) = (int)(pBuffer + 0x20);
                    *(DWORD**)(pBuffer + 0x58) = (DWORD*)(pBuffer + 0x20);
                    *(DWORD*)(pBuffer + 0x20) = (DWORD)(pBuffer + 0x54);
                    *(int**)(pBuffer + 0x24) = (int*)(pBuffer + 0x54);
                    
                    param_1->Control.AutoLogContext = pBuffer + 0x38;
                    return;
                }
            }
            else
            {
                param_1->Control.AutoLogContext = NULL;
            }
        }
        else
        {
            param_1->Control.AutoLogContext = NULL;
        }
    }
    else
    {
        param_1->Control.AutoLogContext = NULL;
    }
}

void __cdecl WppAutoLogStop(WPP_PROJECT_CONTROL_BLOCK* param_1)
{
    param_1->Control.AutoLogContext = NULL;
}

LONG __cdecl WppAutoLogTrace(void* param_1, UCHAR param_2, ULONG param_3, GUID* param_4, USHORT param_5)
{
    if (param_1 == NULL)
    {
        return 0xC0000001;
    }
    
    DWORD* pBufferHeader = (DWORD*)param_1;
    DWORD dwBufferSize = pBufferHeader[0x0E];
    DWORD dwCurrentOffset = pBufferHeader[0x0F];
    DWORD dwTotalSize = 0;
    
    if (dwBufferSize > 0x100)
    {
        dwBufferSize = 0x100;
    }
    
    DWORD dwParamSize = 0;
    DWORD* pParamList = &param_3;
    
    while (*pParamList != 0)
    {
        if (pParamList[1] != 0)
        {
            dwParamSize += (pParamList[1] & 0xFFFF);
        }
        pParamList += 2;
    }
    
    DWORD dwAlignment = (dwParamSize + 0x1C) & 3;
    DWORD dwPadding = (dwAlignment == 0) ? 0 : (4 - dwAlignment);
    DWORD dwTotalEntrySize = dwParamSize + 0x1C + dwPadding;
    
    if (dwTotalEntrySize <= dwBufferSize)
    {
        DWORD dwOldOffset;
        DWORD dwNewOffset;
        USHORT usOldIndex;
        USHORT usNewIndex;
        
        do
        {
            dwOldOffset = dwCurrentOffset;
            
            if ((dwBufferSize + pBufferHeader[0x0D]) < ((dwOldOffset & 0xFFFF) + dwTotalEntrySize + pBufferHeader[0x0D]))
            {
                usOldIndex = 0;
                usNewIndex = (USHORT)dwTotalEntrySize;
            }
            else
            {
                usOldIndex = (USHORT)dwOldOffset;
                usNewIndex = usOldIndex + (USHORT)dwTotalEntrySize;
            }
            
            dwNewOffset = (dwOldOffset & 0xFFFF0000) | usNewIndex;
            
            if (InterlockedCompareExchange((LONG*)&pBufferHeader[0x0F], dwNewOffset, dwOldOffset) == dwOldOffset)
            {
                break;
            }
            
            dwCurrentOffset = pBufferHeader[0x0F];
        } while (dwCurrentOffset != dwOldOffset);
        
        USHORT* pLogEntry = (USHORT*)((BYTE*)param_1 + pBufferHeader[0x0D] + usOldIndex);
        
        pLogEntry[0] = 0x524C;
        pLogEntry[1] = (USHORT)dwTotalEntrySize;
        
        DWORD* pDriverCounter = *(DWORD**)(*(DWORD**)((BYTE*)param_1 + 0x24) + 0x30);
        DWORD dwSequence = InterlockedIncrement((LONG*)pDriverCounter);
        
        *(DWORD*)(pLogEntry + 2) = dwSequence;
        pLogEntry[4] = (USHORT)(dwOldOffset >> 16);
        pLogEntry[5] = param_5;
        
        *(DWORD*)(pLogEntry + 6) = param_4->Data1;
        *(DWORD*)(pLogEntry + 8) = *(DWORD*)&param_4->Data2;
        *(DWORD*)(pLogEntry + 10) = *(DWORD*)param_4->Data4;
        *(DWORD*)(pLogEntry + 12) = *(DWORD*)(param_4->Data4 + 4);
        
        pParamList = &param_3;
        BYTE* pDest = (BYTE*)(pLogEntry + 14);
        
        while (*pParamList != 0)
        {
            if (pParamList[1] != 0)
            {
                DWORD dwCopySize = pParamList[1] & 0xFFFF;
                memcpy(pDest, (void*)pParamList[1], dwCopySize);
                pDest += dwCopySize;
            }
            pParamList += 2;
        }
        
        return 0;
    }
    
    DWORD* pDriverCounter = *(DWORD**)(*(DWORD**)((BYTE*)param_1 + 0x24) + 0x30);
    InterlockedIncrement((LONG*)pDriverCounter);
    
    return 0xC0000001;
}

NTSTATUS __cdecl imp_WppRecorderLogDumpLiveData(
    PVOID WppRecorderContext,
    ULONG DumpFlags,
    PVOID* ppLogBuffer,
    PULONG pBufferSize,
    PLARGE_INTEGER pTimeStamps
)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    
    if (DumpFlags == 0)
    {
        WPP_RECORDER_CONTROL_BLOCK* pRecorderControl = 
            *(WPP_RECORDER_CONTROL_BLOCK**)((BYTE*)WppRecorderContext + 0x28);
        
        if (pRecorderControl != NULL)
        {
            WPP_LOG_BUFFER* pLogBuffer = pRecorderControl->pLogBuffer;
            
            // Check if this is the active buffer for our control block
            if (pRecorderControl == pLogBuffer->pControlBlock)
            {
                // Check if we should return the control block itself or the log buffer
                if (pRecorderControl != pLogBuffer->pControlBlock)
                {
                    // Return control block as buffer (for pending/queued data)
                    *ppLogBuffer = pRecorderControl;
                    *pBufferSize = pRecorderControl->BufferHeaderSize + 0x68;
                }
                else
                {
                    // Return actual log buffer
                    *ppLogBuffer = pLogBuffer;
                    *pBufferSize = pRecorderControl->BufferHeaderSize + 200;
                }
                
                // Return timing info
                pTimeStamps[0].QuadPart = pLogBuffer->StartTimestamp;
                pTimeStamps[1].QuadPart = *(ULONGLONG*)((BYTE*)pLogBuffer + 0x1C);
                
                status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }
    
    return status;
}

