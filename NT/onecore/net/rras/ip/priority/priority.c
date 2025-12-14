/*++

  
Copyright (c) 1995  Microsoft Corporation


Module Name:

    routing\ip\priority\priority.c

Abstract:

    Route Priority DLL

Revision History:

    Gurdeep Singh Pall		7/19/95	Created

--*/

#include <windows.h>
#include <winternl.h>
#include <winerror.h>

typedef struct _PROTOCOL_METRIC_EX {
    DWORD dwProtocolId;
    DWORD dwSubProtocolId;
    DWORD dwMetric;
} PROTOCOL_METRIC_EX, *PPROTOCOL_METRIC_EX;

typedef struct _RTR_TOC_ENTRY {
    DWORD InfoType;
    DWORD InfoSize;
    DWORD Offset;
} RTR_TOC_ENTRY, *PRTR_TOC_ENTRY;

typedef struct _RTR_INFO_BLOCK_HEADER {
    DWORD Version;
    DWORD Size;
    DWORD TocEntriesCount;
    RTR_TOC_ENTRY TocEntry[1];
} RTR_INFO_BLOCK_HEADER, *PRTR_INFO_BLOCK_HEADER;

typedef struct _RoutingProtocolBlock {
    LIST_ENTRY RPB_List;
    PROTOCOL_METRIC_EX RPB_ProtocolMetric;
    // Additional fields follow...
} RoutingProtocolBlock, *PRoutingProtocolBlock;

// Global variables
static CRITICAL_SECTION PriorityLock;
static BOOL bPriorityLockInitialized = FALSE;
static PRoutingProtocolBlock RoutingProtocolBlockPtr = NULL;
static DWORD NumProtocols = 0;
static LIST_ENTRY HashTable[17];  // 0x11 = 17 decimal

// Function prototypes
DWORD ComputeRouteMetricEx(DWORD dwProtocolId, DWORD dwSubProtocolId);

void CleanUpIpPriority(void)
{
    EnterCriticalSection(&PriorityLock);
    
    if (RoutingProtocolBlockPtr != NULL)
    {
        HeapFree(GetProcessHeap(), 0, RoutingProtocolBlockPtr);
        RoutingProtocolBlockPtr = NULL;
    }
    
    LeaveCriticalSection(&PriorityLock);
}

DWORD ComputeRouteMetric(DWORD dwProtocolId)
{
    return ComputeRouteMetricEx(dwProtocolId, 0);
}

DWORD ComputeRouteMetricEx(DWORD dwProtocolId, DWORD dwSubProtocolId)
{
    PLIST_ENTRY pCurrentEntry;
    DWORD dwMetric = 0x7F;  // Default metric
    
    EnterCriticalSection(&PriorityLock);
    
    pCurrentEntry = HashTable[dwProtocolId % 17].Flink;
    
    while (pCurrentEntry != &HashTable[dwProtocolId % 17])
    {
        PRoutingProtocolBlock pProtocolBlock = CONTAINING_RECORD(pCurrentEntry, RoutingProtocolBlock, RPB_List);
        
        if (pProtocolBlock->RPB_ProtocolMetric.dwProtocolId == dwProtocolId &&
            pProtocolBlock->RPB_ProtocolMetric.dwSubProtocolId == dwSubProtocolId)
        {
            dwMetric = pProtocolBlock->RPB_ProtocolMetric.dwMetric;
            break;
        }
        
        pCurrentEntry = pCurrentEntry->Flink;
    }
    
    LeaveCriticalSection(&PriorityLock);
    return dwMetric;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (bPriorityLockInitialized)
        {
            DeleteCriticalSection(&PriorityLock);
            bPriorityLockInitialized = FALSE;
        }
    }
    else if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        InitializeCriticalSection(&PriorityLock);
        bPriorityLockInitialized = TRUE;
        
        // Initialize hash table
        for (int i = 0; i < 17; i++)
        {
            HashTable[i].Flink = &HashTable[i];
            HashTable[i].Blink = &HashTable[i];
        }
    }
    
    return TRUE;
}

DWORD GetPriorityInfo(void* pBuffer, DWORD* pdwBufferSize)
{
    DWORD dwRequiredSize;
    DWORD dwStatus = ERROR_SUCCESS;
    
    EnterCriticalSection(&PriorityLock);
    
    // Calculate required buffer size: NumProtocols * 12 + 4
    if (NumProtocols <= 0xFFFFFFFF / 12)  // Check for overflow
    {
        dwRequiredSize = NumProtocols * 12 + 4;
        
        if (dwRequiredSize >= 4)  // Check for underflow
        {
            DWORD dwProvidedSize = *pdwBufferSize;
            *pdwBufferSize = dwRequiredSize;
            
            if (dwProvidedSize < dwRequiredSize)
            {
                dwStatus = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                // Copy protocol information
                PDWORD pDest = (PDWORD)pBuffer;
                *pDest++ = NumProtocols;
                
                if (NumProtocols > 0)
                {
                    PRoutingProtocolBlock pProtocol = RoutingProtocolBlockPtr;
                    
                    for (DWORD i = 0; i < NumProtocols; i++)
                    {
                        *pDest++ = pProtocol->RPB_ProtocolMetric.dwProtocolId;
                        *pDest++ = pProtocol->RPB_ProtocolMetric.dwSubProtocolId;
                        *pDest++ = pProtocol->RPB_ProtocolMetric.dwMetric;
                        pProtocol++;
                    }
                }
            }
        }
        else
        {
            dwStatus = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }
    }
    else
    {
        dwStatus = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
    }
    
    LeaveCriticalSection(&PriorityLock);
    return dwStatus;
}

DWORD SetPriorityInfo(PRTR_INFO_BLOCK_HEADER pInfoBlock)
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    if (pInfoBlock == NULL)
    {
        return ERROR_SUCCESS;
    }
    
    // Find the TOC entry for protocol priority info (0xffff0017)
    PRTR_TOC_ENTRY pTocEntry = NULL;
    
    for (DWORD i = 0; i < pInfoBlock->TocEntriesCount; i++)
    {
        if (pInfoBlock->TocEntry[i].InfoType == 0xFFFF0017)
        {
            pTocEntry = &pInfoBlock->TocEntry[i];
            break;
        }
    }
    
    if (pTocEntry == NULL)
    {
        return ERROR_SUCCESS;
    }
    
    // Get pointer to the actual data
    PDWORD pData = (PDWORD)((PBYTE)pInfoBlock + pTocEntry->Offset);
    
    if (pTocEntry->Offset > pInfoBlock->Size)
    {
        pData = NULL;
    }
    
    if (pData == NULL)
    {
        return ERROR_SUCCESS;
    }
    
    EnterCriticalSection(&PriorityLock);
    
    // Clean up existing data
    if (RoutingProtocolBlockPtr != NULL)
    {
        HeapFree(GetProcessHeap(), 0, RoutingProtocolBlockPtr);
        RoutingProtocolBlockPtr = NULL;
        
        // Reset hash table
        for (int i = 0; i < 17; i++)
        {
            HashTable[i].Flink = &HashTable[i];
            HashTable[i].Blink = &HashTable[i];
        }
    }
    
    if (pTocEntry->InfoSize != 0)
    {
        DWORD dwProtocolCount = *pData;
        
        // Check for overflow: dwProtocolCount * 20
        if (dwProtocolCount <= 0xFFFFFFFF / 20)
        {
            SIZE_T dwAllocSize = dwProtocolCount * sizeof(RoutingProtocolBlock);
            PRoutingProtocolBlock pNewBlock = (PRoutingProtocolBlock)HeapAlloc(
                GetProcessHeap(), HEAP_ZERO_MEMORY, dwAllocSize);
            
            RoutingProtocolBlockPtr = pNewBlock;
            
            if (pNewBlock != NULL)
            {
                NumProtocols = dwProtocolCount;
                pData++;  // Skip protocol count
                
                for (DWORD i = 0; i < dwProtocolCount; i++)
                {
                    pNewBlock->RPB_ProtocolMetric.dwProtocolId = *pData++;
                    pNewBlock->RPB_ProtocolMetric.dwSubProtocolId = *pData++;
                    pNewBlock->RPB_ProtocolMetric.dwMetric = *pData++;
                    
                    // Insert into hash table
                    DWORD dwHashIndex = pNewBlock->RPB_ProtocolMetric.dwProtocolId % 17;
                    
                    pNewBlock->RPB_List.Flink = &HashTable[dwHashIndex];
                    pNewBlock->RPB_List.Blink = HashTable[dwHashIndex].Blink;
                    
                    HashTable[dwHashIndex].Blink->Flink = &pNewBlock->RPB_List;
                    HashTable[dwHashIndex].Blink = &pNewBlock->RPB_List;
                    
                    pNewBlock++;
                }
            }
            else
            {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            dwStatus = ERROR_ARITHMETIC_OVERFLOW;
        }
    }
    
    LeaveCriticalSection(&PriorityLock);
    return dwStatus;
}

