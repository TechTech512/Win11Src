#include <windows.h>

/* External functions - assumed to exist in other source files */
extern int MmeCache_DiscardLru(void);

void *lRealloc(void *ptr, unsigned int size, int param_3, unsigned int param_4, int param_5)
{
    HANDLE heapHandle;
    void *newPtr;
    int discardResult;
    
    do {
        heapHandle = GetProcessHeap();
        newPtr = HeapReAlloc(heapHandle, 0, ptr, size);
        if (newPtr != NULL) {
            return newPtr;
        }
        discardResult = MmeCache_DiscardLru();
    } while (discardResult != 0);
    
    heapHandle = GetProcessHeap();
    HeapFree(heapHandle, 0, ptr);
    return NULL;
}

typedef enum t_HeapUsage {
    eHeapUsageUnknown = 0,
    eHeapUsageTemp = 1,
    eHeapUsageTaskDef = 2,
    eHeapUsageString = 3,
    eHeapUsageWorkspace = 4,
    eHeapUsageGraphics = 5,
    eHeapUsageURL = 6,
    eHeapUsageMBuf = 7,
    eHeapUsageDisplayList = 8,
    eHeapUsageCacheList = 9,
    eHeapUsageDebugHarness = 10,
    eHeapUsageWtlDocument = 11,
    eHeapUsageWtlPipeData = 12,
    eHeapUsageDocumentTokenStart = 13,
    eHeapUsageDocumentTokenHTML = 13,
    eHeapUsageDocumentTokenImage = 14,
    eHeapUsageDocumentTokenROMImage = 15,
    eHeapUsageDocumentTokenNotify = 16,
    eHeapUsageDocumentTokenAnim = 17,
    eHeapUsageDocumentTokenROMAnim = 18,
    eHeapUsageDocumentTokenEnd = 18,
    eHeapUsageConfigData = 19,
    eHeapUsageRenderElement = 20,
    eHeapUsageRomFileStruct = 21,
    eHeapUsageUrlOpenDocument = 22,
    eHeapUsageWtlDocumentHeaders = 23,
    eHeapUsageRenderQueue = 24,
    eHeapUsageTableRender = 25,
    eHeapUsagePwlWindow = 26,
    eHeapUsagePwlTimer = 27,
    eHeapUsagePwlObject = 28,
    eHeapUsageImmContext = 29,
    eHeapUsageECAPI = 30,
    eHeapUsageWordWrap = 31,
    eHeapUsageSSL = 32,
    eHeapUsageWTLS = 33,
    eHeapUsageSockMsg = 34,
    eHeapUsageSms = 35,
    eHeapUsageHttpUnknown = 36,
    eHeapUsageWspUnknown = 37,
    eHeapUsageMailUnknown = 38,
    eHeapUsageMailToUnknown = 39,
    eHeapUsageWtlUtilUnknown = 40,
    eHeapUsageIPStackUnknown = 41,
    eHeapUsageAccesskeyList = 42,
    eHeapUsageAccesskeys = 43,
    eHeapUsageSsDir = 44,
    eHeapUsageSsPair = 45,
    eHeapUsageSstoreUnknown = 46,
    eHeapUsageJpegHuffman = 47,
    eHeapUsageAuthentication = 48,
    eHeapUsageCookie = 49,
    eHeapUsageConfig = 50,
    eHeapUsageGlue = 51,
    eHeapUsageDebug = 52,
    eHeapUsageEnd = 53
} t_HeapUsage;

void *UBHeap_Alloc(unsigned int size, t_HeapUsage usage, unsigned int param_3)
{
    HANDLE heapHandle;
    void *allocatedPtr;
    int discardResult;
    
    do {
        heapHandle = GetProcessHeap();
        allocatedPtr = HeapAlloc(heapHandle, 0, size);
        if (allocatedPtr != NULL) {
            return allocatedPtr;
        }
        discardResult = MmeCache_DiscardLru();
    } while (discardResult != 0);
    
    return NULL;
}

void *UBHeap_AllocZero(unsigned int size, t_HeapUsage usage, unsigned int param_3)
{
    HANDLE heapHandle;
    void *allocatedPtr;
    int discardResult;
    
    do {
        heapHandle = GetProcessHeap();
        allocatedPtr = HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, size);
        if (allocatedPtr != NULL) {
            return allocatedPtr;
        }
        discardResult = MmeCache_DiscardLru();
    } while (discardResult != 0);
    
    return NULL;
}

void UBHeap_Free(void *ptr, unsigned int param_2)
{
    HANDLE heapHandle;
    
    if (ptr != NULL) {
        heapHandle = GetProcessHeap();
        HeapFree(heapHandle, 0, ptr);
    }
}

