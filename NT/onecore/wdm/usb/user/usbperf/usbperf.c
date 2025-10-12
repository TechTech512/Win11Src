#include <windows.h>
#include <winperf.h>
#include <wbemidl.h>
#include <wmistr.h>
#include <evntprov.h>
#include <strsafe.h>

// Global variables
LONG OpenCount = 0;
HANDLE EventLog = NULL;
BOOL DllInit = FALSE;
BOOL LogCollectError = FALSE;

// Performance counter definitions
#define NUM_USB_COUNTERS 19

// Performance data structure
PERF_OBJECT_TYPE UsbPerfData = {
    sizeof(PERF_OBJECT_TYPE) + (NUM_USB_COUNTERS * sizeof(PERF_COUNTER_DEFINITION)), // TotalByteLength
    sizeof(PERF_OBJECT_TYPE),                    // DefinitionLength  
    sizeof(PERF_OBJECT_TYPE),                    // HeaderLength
    0,                                          // ObjectNameTitleIndex
    0,                                          // ObjectNameTitle
    0,                                          // ObjectHelpTitleIndex
    0,                                          // ObjectHelpTitle
    0,                                          // DetailLevel
    NUM_USB_COUNTERS,                           // NumCounters
    0,                                          // DefaultCounter
    0,                                          // NumInstances
    0,                                          // CodePage
    {0, 0}                                      // PerfTime
};

PERF_COUNTER_DEFINITION UsbCounters[NUM_USB_COUNTERS] = {
    // Bulk Urbs Counter
    { sizeof(PERF_COUNTER_DEFINITION), 1, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Bulk Bytes Counter  
    { sizeof(PERF_COUNTER_DEFINITION), 2, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Isoch Urbs Counter
    { sizeof(PERF_COUNTER_DEFINITION), 3, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Isoch Bytes Counter
    { sizeof(PERF_COUNTER_DEFINITION), 4, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Interrupt Urbs Counter
    { sizeof(PERF_COUNTER_DEFINITION), 5, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Interrupt Bytes Counter
    { sizeof(PERF_COUNTER_DEFINITION), 6, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Control Urbs Counter
    { sizeof(PERF_COUNTER_DEFINITION), 7, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Control Bytes Counter
    { sizeof(PERF_COUNTER_DEFINITION), 8, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // PciInterrupt Counter
    { sizeof(PERF_COUNTER_DEFINITION), 9, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // WorkSignals Counter
    { sizeof(PERF_COUNTER_DEFINITION), 10, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Interrupt Bandwidth Usage
    { sizeof(PERF_COUNTER_DEFINITION), 11, 0, 0, 0, 0, 0x00000020, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWFRACTION
    // Isoch Bandwidth Usage
    { sizeof(PERF_COUNTER_DEFINITION), 12, 0, 0, 0, 0, 0x00000020, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWFRACTION
    // Bytes Per Transfer
    { sizeof(PERF_COUNTER_DEFINITION), 13, 0, 0, 0, 0, 0x00040000, sizeof(DWORD), 0 }, // PERF_COUNTER_AVERAGE_BASE
    // Bytes Per Transfer Base
    { sizeof(PERF_COUNTER_DEFINITION), 0, 0, 0, 0, 0, 0x00040000, sizeof(DWORD), 0 }, // PERF_COUNTER_AVERAGE_BASE
    // Dropped Iso Packets
    { sizeof(PERF_COUNTER_DEFINITION), 14, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // Average Iso Latency
    { sizeof(PERF_COUNTER_DEFINITION), 15, 0, 0, 0, 0, 0x00040000, sizeof(DWORD), 0 }, // PERF_COUNTER_AVERAGE_BASE
    // Average Iso Latency Base
    { sizeof(PERF_COUNTER_DEFINITION), 0, 0, 0, 0, 0, 0x00040000, sizeof(DWORD), 0 }, // PERF_COUNTER_AVERAGE_BASE
    // Transfer Errors
    { sizeof(PERF_COUNTER_DEFINITION), 16, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }, // PERF_COUNTER_RAWCOUNT
    // HC Idle State
    { sizeof(PERF_COUNTER_DEFINITION), 17, 0, 0, 0, 0, 0x00000400, sizeof(DWORD), 0 }  // PERF_COUNTER_RAWCOUNT
};

// Function declarations
UCHAR BuildDeviceInstance(PERF_INSTANCE_DEFINITION** ppInstance, WNODE_ALL_DATA* pWnode);
DWORD CloseUsbPerformanceData();
DWORD CollectUsbPerformanceData(LPWSTR lpValueName, LPVOID* lppData, LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes);
BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
DWORD OpenUsbPerformanceData(LPWSTR lpValueName);

extern DWORD WINAPI WmiOpenBlock(GUID* Guid, DWORD DesiredAccess, HANDLE* DataBlockHandle);
extern DWORD WINAPI WmiQueryAllDataW(HANDLE DataBlockHandle, DWORD* BufferSize, WNODE_ALL_DATA* DataBlock);
extern DWORD WINAPI WmiCloseBlock(HANDLE DataBlockHandle);

// External functions from performance helper library
extern DWORD GetQueryType(LPWSTR lpValueName);
extern int MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION* pInstance,
    LPVOID* ppBuffer,
    DWORD dwBufferSize,
    DWORD dwInstanceName,
    DWORD dwParentInstance,
    LPWSTR pszInstanceName);

UCHAR BuildDeviceInstance(PERF_INSTANCE_DEFINITION** ppInstance, WNODE_ALL_DATA* pWnode)
{
    if (!pWnode || !ppInstance || !*ppInstance) {
        return FALSE;
    }

    DWORD* pDataBlock = (DWORD*)((BYTE*)pWnode + pWnode->DataBlockOffset);
    WCHAR* pInstanceName = (WCHAR*)(pDataBlock + 0x11);
    WCHAR szSanitizedName[60] = {0};
    DWORD dwNameLength = 0;

    // Sanitize instance name - replace forward slashes with hyphens
    if (pInstanceName && *pInstanceName != L'\0') {
        for (DWORD i = 0; i < 59; i++) {
            WCHAR wch = pInstanceName[i];
            if (wch == L'\0') {
                break;
            }
            if (wch == L'/') {
                wch = L'-';
            }
            szSanitizedName[i] = wch;
            dwNameLength++;
        }
        szSanitizedName[dwNameLength] = L'\0';
    }

    // If no instance name in data block, get from fixed offset
    if (dwNameLength == 0) {
        pInstanceName = (WCHAR*)((BYTE*)pWnode + 2 + *(BYTE*)((BYTE*)pWnode + pWnode->DataBlockOffset + 0x38));
        if (pInstanceName) {
            StringCchCopyW(szSanitizedName, 60, pInstanceName);
        }
    }

    // Build the instance definition
    PERF_INSTANCE_DEFINITION* pInstance = *ppInstance;
    if (MonBuildInstanceDefinition(pInstance, NULL, 0, (DWORD)szSanitizedName, 0, szSanitizedName) != 0) {
        return FALSE;
    }

    // Get pointer to counter data following instance definition
    DWORD* pCounterData = (DWORD*)((BYTE*)pInstance + pInstance->ByteLength);

    // Initialize counter block header
    pCounterData[0] = 0x60;  // Total byte length of counter block

    // Copy basic counter values
    pCounterData[2] = pDataBlock[0];   // Counter 1
    pCounterData[3] = pDataBlock[2];   // Counter 2  
    pCounterData[4] = pDataBlock[3];   // Counter 3
    pCounterData[5] = pDataBlock[1];   // Counter 4
    pCounterData[0xD] = pDataBlock[0x31]; // Counter 13
    pCounterData[0x11] = pDataBlock[0x32]; // Counter 17

    // Calculate derived counter values
    pCounterData[0xC] = pDataBlock[7] + pDataBlock[5] + pDataBlock[4] + pDataBlock[6]; // Sum of counters 4-7
    pCounterData[0x10] = pDataBlock[6]; // Counter 6 directly
    pCounterData[10] = pDataBlock[2] + pDataBlock[1] + pDataBlock[0] + pDataBlock[3]; // Sum of counters 0-3

    // Initialize unused counters
    pCounterData[0xB] = 0;
    pCounterData[0xF] = 0;
    pCounterData[7] = 0;
    pCounterData[9] = 0;
    pCounterData[8] = 0;

    // Copy additional counter values
    pCounterData[0xE] = pDataBlock[0x30];
    pCounterData[6] = pDataBlock[0x33];
    pCounterData[0x12] = pDataBlock[0x34];
    pCounterData[0x13] = pDataBlock[0x35];
    pCounterData[0x14] = pDataBlock[0x36];
    pCounterData[0x15] = pDataBlock[0x37];
    pCounterData[0x16] = pDataBlock[0x38];

    // Calculate percentage counters if denominator is non-zero
    if (pDataBlock[0xF] != 0) {
        pCounterData[9] = (pDataBlock[0xE] * 100) / pDataBlock[0xF];
        pCounterData[8] = ((pDataBlock[0xD] + pDataBlock[0xC] + pDataBlock[0xB] + 
                           pDataBlock[0xA] + pDataBlock[9] + pDataBlock[8]) * 100) / pDataBlock[0xF];
    }

    // Update instance pointer to point past this counter block
    *ppInstance = (PERF_INSTANCE_DEFINITION*)(pCounterData + 0x18);
    
    return TRUE;
}

DWORD CloseUsbPerformanceData()
{
    LONG currentCount = InterlockedDecrement(&OpenCount);
    
    if (currentCount == 0 && EventLog != NULL) {
        DeregisterEventSource(EventLog);
        EventLog = NULL;
    }
    
    return ERROR_SUCCESS;
}

DWORD CollectUsbPerformanceData(LPWSTR lpValueName, LPVOID* lppData, LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes)
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwRequiredSize = 0x338;  // Initial size for base performance data
    BYTE* pOutputBuffer = NULL;
    BYTE* pCurrentPosition = NULL;
    HANDLE hWmiBlock = NULL;
    DWORD dwInstanceCount = 0;
    WNODE_ALL_DATA* pWmiData = NULL;
    DWORD dwWmiBufferSize = 0xE4;
    BOOL bProcessingComplete = FALSE;

    // Initialize output parameters
    if (!lppData || !lpcbTotalBytes || !lpNumObjectTypes) {
        return ERROR_INVALID_PARAMETER;
    }

    // Check if DLL is properly initialized
    if (!DllInit) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        
        if (!LogCollectError && EventLog) {
            LogCollectError = TRUE;
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D3, NULL, 0, 0, NULL, NULL);
        }
        return dwStatus;
    }

    // Determine query type
    DWORD dwQueryType = GetQueryType(lpValueName);
    if (dwQueryType == 3 || dwQueryType == 4) {  // Foreign or costly data
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        
        if (!LogCollectError && EventLog) {
            LogCollectError = TRUE;
        }
        return dwStatus;
    }

    // Verify buffer is large enough for base data
    if (*lpcbTotalBytes < dwRequiredSize) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

    pOutputBuffer = (BYTE*)*lppData;
    pCurrentPosition = pOutputBuffer;

    // Copy base performance object structure and counter definitions
    memcpy(pCurrentPosition, &UsbPerfData, sizeof(UsbPerfData));
    pCurrentPosition += sizeof(UsbPerfData);
    
    // Copy counter definitions
    memcpy(pCurrentPosition, UsbCounters, sizeof(UsbCounters));
    pCurrentPosition += sizeof(UsbCounters);

    // Define USB performance GUID
    GUID guidUsbPerformance = {
        0x3C6C6F66,     // Data1
        0x9F49,         // Data2  
        0x9F49,         // Data3
        {0xA0, 0xA9, 0xA5, 0x61, 0xE2, 0x35, 0x9F, 0x64}  // Data4
    };

    // Open WMI block for USB performance data
    dwStatus = WmiOpenBlock(&guidUsbPerformance, 0x80000000, &hWmiBlock);
    
    if (dwStatus == ERROR_SUCCESS && hWmiBlock != NULL) {
        // Allocate initial buffer for WMI data
        pWmiData = (WNODE_ALL_DATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwWmiBufferSize);
        
        // Query WMI data with dynamic buffer sizing
        while (pWmiData != NULL) {
            dwStatus = WmiQueryAllDataW(hWmiBlock, &dwWmiBufferSize, pWmiData);
            
            if (dwStatus == ERROR_SUCCESS) {
                if (dwWmiBufferSize > 0xE3) {
                    // Process each data instance in the WMI block
                    DWORD dwCurrentOffset = pWmiData->DataBlockOffset;
                    bProcessingComplete = FALSE;
                    
                    while (!bProcessingComplete && dwCurrentOffset != 0) {
                        WNODE_ALL_DATA* pCurrentInstance = (WNODE_ALL_DATA*)((BYTE*)pWmiData + dwCurrentOffset);
                        
                        // Build performance instance for this device
                        PERF_INSTANCE_DEFINITION* pInstanceDef = (PERF_INSTANCE_DEFINITION*)pCurrentPosition;
                        UCHAR bInstanceResult = BuildDeviceInstance(&pInstanceDef, pCurrentInstance);
                        
                        if (bInstanceResult) {
                            dwInstanceCount++;
                            pCurrentPosition = (BYTE*)pInstanceDef;
                            
                            // Update required size estimate
                            BYTE nameLength = *(BYTE*)((BYTE*)pCurrentInstance + pCurrentInstance->DataBlockOffset + 0x38);
                            dwRequiredSize += (nameLength * 2) + 0x78;
                            
                            // Check if we have enough buffer space
                            if ((DWORD)(pCurrentPosition - pOutputBuffer) + dwRequiredSize > *lpcbTotalBytes) {
                                *lpcbTotalBytes = 0;
                                *lpNumObjectTypes = 0;
                                
                                if (hWmiBlock) {
                                    WmiCloseBlock(hWmiBlock);
                                }
                                if (pWmiData) {
                                    HeapFree(GetProcessHeap(), 0, pWmiData);
                                }
                                return ERROR_MORE_DATA;
                            }
                        }
                        
                        // Move to next instance
                        if (pCurrentInstance->WnodeHeader.Linkage == 0) {
                            bProcessingComplete = TRUE;
                        } else {
                            dwCurrentOffset = pCurrentInstance->WnodeHeader.Linkage;
                        }
                    }
                    break; // Successfully processed all instances
                }
            } else if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
                // Reallocate larger buffer and try again
                HeapFree(GetProcessHeap(), 0, pWmiData);
                pWmiData = (WNODE_ALL_DATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwWmiBufferSize);
                continue;
            } else {
                // Other error - break out
                break;
            }
        }
        
        // Close WMI block
        if (hWmiBlock != NULL) {
            WmiCloseBlock(hWmiBlock);
        }
    }

    // Calculate final data size
    DWORD dwTotalSize = (DWORD)(pCurrentPosition - pOutputBuffer);
    
    // Update performance object with instance count
    PERF_OBJECT_TYPE* pPerfObject = (PERF_OBJECT_TYPE*)pOutputBuffer;
    pPerfObject->NumInstances = dwInstanceCount;
    pPerfObject->TotalByteLength = dwTotalSize;

    // Set return values
    *lppData = pOutputBuffer;
    *lpcbTotalBytes = dwTotalSize;
    *lpNumObjectTypes = 1;  // Single object type (USB)

    // Cleanup WMI data buffer
    if (pWmiData != NULL) {
        HeapFree(GetProcessHeap(), 0, pWmiData);
    }

    return dwStatus;
}

BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            if (EventLog != NULL) {
                DeregisterEventSource(EventLog);
                EventLog = NULL;
            }
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    
    return TRUE;
}

DWORD OpenUsbPerformanceData(LPWSTR lpValueName)
{
    UNREFERENCED_PARAMETER(lpValueName);

    DWORD dwStatus = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwFirstCounter = 0;
    DWORD dwFirstHelp = 0;
    DWORD dwLastCounter = 0;
    DWORD dwLastHelp = 0;
    DWORD dwValueType = 0;
    DWORD dwValueSize = sizeof(DWORD);

    // Increment open count
    LONG currentCount = InterlockedIncrement(&OpenCount);
    
    // If already open, just return success
    if (currentCount != 1) {
        return dwStatus;
    }

    // Register event source for error reporting
    if (EventLog == NULL) {
        EventLog = RegisterEventSourceA(NULL, "usbperf");
    }

    // Open performance registry key
    dwStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                           "SYSTEM\\CurrentControlSet\\Services\\usbhub\\Performance",
                           0, KEY_READ, &hKey);

    if (dwStatus != ERROR_SUCCESS) {
        if (EventLog) {
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D0, NULL, 0, 0, NULL, NULL);
        }
        goto cleanup;
    }

    // Read First Counter value
    dwStatus = RegQueryValueExA(hKey, "First Counter", NULL, &dwValueType, 
                              (LPBYTE)&dwFirstCounter, &dwValueSize);
    if (dwStatus != ERROR_SUCCESS) {
        if (EventLog) {
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D1, NULL, 0, 0, NULL, NULL);
        }
        goto cleanup;
    }

    // Read First Help value  
    dwStatus = RegQueryValueExA(hKey, "First Help", NULL, &dwValueType,
                              (LPBYTE)&dwFirstHelp, &dwValueSize);
    if (dwStatus != ERROR_SUCCESS) {
        if (EventLog) {
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D1, NULL, 0, 0, NULL, NULL);
        }
        goto cleanup;
    }

    // Read Last Counter value
    dwStatus = RegQueryValueExA(hKey, "Last Counter", NULL, &dwValueType,
                              (LPBYTE)&dwLastCounter, &dwValueSize);
    if (dwStatus != ERROR_SUCCESS) {
        if (EventLog) {
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D2, NULL, 0, 0, NULL, NULL);
        }
        goto cleanup;
    }

    // Read Last Help value
    dwStatus = RegQueryValueExA(hKey, "Last Help", NULL, &dwValueType,
                              (LPBYTE)&dwLastHelp, &dwValueSize);
    if (dwStatus != ERROR_SUCCESS) {
        if (EventLog) {
            ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D2, NULL, 0, 0, NULL, NULL);
        }
        goto cleanup;
    }

    // Update performance object with counter offsets
    UsbPerfData.ObjectNameTitleIndex += dwFirstCounter;
    UsbPerfData.ObjectHelpTitleIndex += dwFirstHelp;
    
    // Update all counter definitions with offsets
    for (DWORD i = 0; i < NUM_USB_COUNTERS; i++) {
        UsbCounters[i].CounterNameTitleIndex += dwFirstCounter;
        UsbCounters[i].CounterHelpTitleIndex += dwFirstHelp;
        
        // Validate counter indices are within range
        if (UsbCounters[i].CounterNameTitleIndex > dwLastCounter || 
            UsbCounters[i].CounterHelpTitleIndex > dwLastHelp) {
            if (EventLog) {
                ReportEventA(EventLog, EVENTLOG_ERROR_TYPE, 0, 0xC00007D8, NULL, 0, 0, NULL, NULL);
            }
            dwStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    // Set default counter
    UsbPerfData.DefaultCounter = 0;

    // Mark as initialized
    DllInit = TRUE;

cleanup:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    
    return dwStatus;
}

