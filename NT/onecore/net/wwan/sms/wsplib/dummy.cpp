#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>

/* Structure definitions */
typedef struct s_ParsedURL {
    char *user;
    char *password;
    char *scheme;
    char *host;
    unsigned int port;
    char *path;
    char *params;
    char *query;
    char *fragment;
    unsigned int iUrlLength;
    int iUrlHasScheme;
    int iUrlHasNetLoc;
    int iUrlHasPort;
    char iUrlComponents[1];
} s_ParsedURL;

typedef struct s_URLDocument {
    s_ParsedURL *base;
    s_ParsedURL *request;
    int iOriginTrustLevel;
    int iTrustLevel;
    /* Other members... */
    unsigned long flags;
    unsigned long pending_flags;
    unsigned long size_available;
    void *headers_list;  /* Offset 0x70 */
    void *body_buffer;   /* Offset 0x6c */
    void *type_fifo_head; /* Offset 0xa0 */
    void *type_fifo_tail; /* Offset 0xa4 */
    /* Other members... */
} s_URLDocument;

typedef struct MBuf {
    struct MBuf *next;
    int iOwners;
    int length;
    int start;
    unsigned char buf[50];
    struct MBuf *next_packet;
    unsigned char old_start;
    unsigned char retry_time;
} MBuf;

/* Enum definitions */
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

typedef enum t_UrlTrustLevel {
    eUrlTrustNone = 0,
    eUrlTrustLocal = 1
} t_UrlTrustLevel;

/* External functions */
extern "C" void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern "C" void *UBHeap_AllocZero(unsigned int size, int usage, unsigned int param_3);
extern "C" void UBHeap_Free(void *ptr, unsigned int param_2);
extern "C" void *lRealloc(void *ptr, unsigned int size, int param_3, unsigned int param_4, int param_5);
extern "C" long lUrl_Parse(s_ParsedURL **result, char *urlString, int param_3, int param_4);
extern "C" s_URLDocument *MmeCache_FindDocument(s_ParsedURL *url);
extern "C" void MmeCache_DeleteDocument(s_URLDocument *document);
extern "C" long MBufC_CopyOut(MBuf *mbufChain, unsigned char *buffer, int offset, int size);
extern unsigned int __stdcall ExtractDocumentData(s_URLDocument *result, unsigned char **dataBuffer, unsigned long *dataSize);
extern unsigned int __stdcall WSPGetMIMEType(unsigned long token, wchar_t *outputBuffer);

long AddHeader(wchar_t **headerName, char *headerValue, char *param_3)
{
    wchar_t *existingBuffer;
    wchar_t *newBuffer;
    wchar_t *tempBuffer1;
    wchar_t *tempBuffer2;
    void *allocatedBuffer;
    int nameLength;
    int valueLength;
    long result;
    
    /* Calculate lengths */
    if (headerValue == NULL) {
        valueLength = 0;
        nameLength = 0;
    }
    else {
        valueLength = (int)strlen(headerValue);
        if (headerName != NULL) {
            wchar_t **temp = headerName;
            while (*temp != L'\0') {
                temp++;
            }
            nameLength = (int)((wchar_t **)temp - headerName);
        }
        else {
            nameLength = 0;
        }
    }
    
    /* Get existing buffer or allocate new one */
    existingBuffer = NULL;  /* This would come from a parameter */
    if (existingBuffer == NULL) {
        /* Allocate new buffer */
        allocatedBuffer = UBHeap_AllocZero(0, eHeapUsageString, 0);
        newBuffer = (wchar_t *)allocatedBuffer;
    }
    else {
        /* Reallocate existing buffer */
        wchar_t *temp = existingBuffer;
        while (*temp != L'\0') {
            temp++;
        }
        int existingLength = (int)(temp - existingBuffer);
        
        allocatedBuffer = lRealloc(NULL, existingLength + valueLength + nameLength + 10, 0, 0, 0);
        newBuffer = (wchar_t *)allocatedBuffer;
    }
    
    if (newBuffer == NULL) {
        return -0x7FF2FFFF;
    }
    
    /* Build header string */
    if (headerValue == NULL) {
        /* Just add CRLF */
        result = StringCchCatW(newBuffer, 0, L"\r\n");
        if (result >= 0) {
            result = StringCchCatW(newBuffer, 0, L"\r\n");
            if (result >= 0) {
                result = 0;
            }
            else {
                result = -0x7FF2FFFF;
            }
        }
        else {
            result = -0x7FF2FFFF;
        }
    }
    else {
        /* Convert header name and value to wide char and concatenate */
        tempBuffer1 = (wchar_t *)UBHeap_Alloc((valueLength + 2) * sizeof(wchar_t), eHeapUsageString, 0);
        if (tempBuffer1 == NULL) {
            result = -0x7FF2FFFF;
            goto cleanup;
        }
        
        tempBuffer2 = (wchar_t *)UBHeap_Alloc((nameLength + 2) * sizeof(wchar_t), eHeapUsageString, 0);
        if (tempBuffer2 == NULL) {
            UBHeap_Free(tempBuffer1, 0);
            result = -0x7FF2FFFF;
            goto cleanup;
        }
        
        /* Convert to wide char */
        MultiByteToWideChar(CP_ACP, 0, headerValue, -1, tempBuffer1, valueLength + 1);
        if (headerName != NULL) {
            /* Header name is already wide char, just copy */
            wcsncpy(tempBuffer2, (const wchar_t*)headerName, nameLength + 1);
        }
        else {
            tempBuffer2[0] = L'\0';
        }
        
        /* Build full header: "Name: Value\r\n" */
        result = StringCchCatW(tempBuffer2, nameLength + valueLength + 10, tempBuffer1);
        if (result >= 0) {
            result = StringCchCatW(tempBuffer2, nameLength + valueLength + 10, L": ");
            if (result >= 0) {
                result = StringCchCatW(tempBuffer1, valueLength + nameLength + 10, tempBuffer2);
                if (result >= 0) {
                    result = StringCchCatW(tempBuffer1, valueLength + nameLength + 10, L"\r\n");
                    if (result >= 0) {
                        /* Success */
                        result = 0;
                    }
                }
            }
        }
        
        if (result < 0) {
            result = -0x7FF2FFFF;
        }
        
        UBHeap_Free(tempBuffer2, 0);
        UBHeap_Free(tempBuffer1, 0);
    }
    
cleanup:
    return result;
}

long GetPushedHeaders(s_URLDocument *document, unsigned long param_2, wchar_t **headerBuffer)
{
    char mimeTypeBuffer[100];
    char *tempBuffer;
    wchar_t *wideBuffer;
    int bufferSize;
    int length;
    long result;
    
    /* Security cookie check */
    unsigned int savedCookie = __security_cookie;
    
    /* Initialize buffer */
    memset(mimeTypeBuffer, 0, sizeof(mimeTypeBuffer));
    document->base = NULL;
    
    /* Add Content-Length header if param_2 is not zero */
    if (param_2 != 0) {
        char lengthStr[20];
        if (_itoa_s((int)param_2, lengthStr, sizeof(lengthStr), 10) != 0) {
            result = 1;
            goto cleanup;
        }
        
        AddHeader((wchar_t**)L"Content-Length", lengthStr, NULL);
    }
    
    /* Get MIME type */
    WSPGetMIMEType(param_2, (wchar_t *)mimeTypeBuffer);
    
    /* Calculate buffer size needed */
    wchar_t *temp = (wchar_t *)mimeTypeBuffer;
    length = 0;
    while (*temp != L'\0') {
        temp++;
        length++;
    }
    
    bufferSize = length + 1;
    tempBuffer = (char *)UBHeap_Alloc(bufferSize, eHeapUsageString, 0);
    if (tempBuffer != NULL) {
        /* Convert wide char to multi-byte */
        WideCharToMultiByte(CP_ACP, 0, (wchar_t *)mimeTypeBuffer, -1, 
                           tempBuffer, bufferSize, NULL, NULL);
        
        /* Add Content-Type header */
        AddHeader((wchar_t**)L"Content-Type", tempBuffer, NULL);
        
        UBHeap_Free(tempBuffer, 0);
    }
    
    /* Add all headers from document's header list */
    void **headerList = (void **)document->headers_list;
    while (headerList != NULL) {
        /* Add each header */
        wchar_t **headerName = (wchar_t **)((char *)headerList + 12);  /* Offset to header name */
        char *headerValue = (char *)(headerList + 3);  /* Offset to header value */
        
        AddHeader(headerName, headerValue, NULL);
        
        headerList = (void **)*headerList;  /* Next in list */
    }
    
    /* Add terminating CRLF */
    AddHeader(NULL, NULL, NULL);
    
    result = 0;
    
cleanup:
    __security_check_cookie(savedCookie);
    return result;
}

long Notify_NewDocument(char *url, unsigned char **documentData, unsigned long *dataSize, wchar_t **headers)
{
    s_ParsedURL *parsedUrl = NULL;
    s_URLDocument *cachedDocument = NULL;
    unsigned char *extractedData = NULL;
    unsigned long extractedSize = 0;
    wchar_t *headerBuffer = NULL;
    char *dataBuffer = NULL;
    wchar_t *wideHeaderBuffer = NULL;
    unsigned long resultCode;
    long parseResult;
    unsigned long extractResult;
    long headerResult;
    
    /* Parse the URL */
    parseResult = lUrl_Parse(&parsedUrl, url, 0, 0);
    if (parseResult != 0) {
        resultCode = 0x80004005;  /* E_FAIL */
        goto cleanup;
    }
    
    /* Look for document in cache */
    cachedDocument = MmeCache_FindDocument(parsedUrl);
    if (cachedDocument == NULL) {
        resultCode = 0x8007000E;  /* E_OUTOFMEMORY */
        goto cleanup;
    }
    
    /* Extract document data */
    extractResult = ExtractDocumentData((s_URLDocument *)&parsedUrl, &extractedData, &extractedSize);
    if ((int)extractResult < 0) {
        resultCode = extractResult;
        goto cleanup;
    }
    
    /* Get headers */
    headerResult = GetPushedHeaders(cachedDocument, extractedSize, &headerBuffer);
    if (headerResult != 0) {
        resultCode = 0x80004005;  /* E_FAIL */
        goto cleanup;
    }
    
    /* Allocate buffer for document data */
    if (dataBuffer == NULL) {
        resultCode = 0x8007000E;  /* E_OUTOFMEMORY */
        goto cleanup;
    }
    
    /* Copy data */
    memcpy(dataBuffer, extractedData, extractedSize);
    
    /* Store parsed URL */
    *(s_ParsedURL **)url = parsedUrl;
    
    /* Calculate header buffer size and allocate */
    if (headerBuffer == NULL) {
        /* No headers */
        if (wideHeaderBuffer != NULL) {
            wideHeaderBuffer[0] = L'\0';
        }
    }
    else {
        /* Calculate length and allocate */
        wchar_t *temp = headerBuffer;
        while (*temp != L'\0') {
            temp++;
        }
        int headerLength = (int)(temp - headerBuffer);
        
        if (wideHeaderBuffer != NULL) {
            StringCchCopyW(wideHeaderBuffer, headerLength + 1, headerBuffer);
        }
    }
    
    if (wideHeaderBuffer == NULL) {
        dataBuffer = NULL;
        resultCode = 0x8007000E;  /* E_OUTOFMEMORY */
        goto cleanup;
    }
    
    /* Return values */
    *documentData = (unsigned char *)dataBuffer;
    *dataSize = extractedSize;
    *headers = wideHeaderBuffer;
    
    resultCode = 0;  /* S_OK */
    
cleanup:
    /* Clean up resources */
    if (parsedUrl != NULL) {
        UBHeap_Free(parsedUrl, 0);
    }
    
    if (extractedData != NULL) {
        UBHeap_Free(extractedData, 0);
    }
    
    if (headerBuffer != NULL) {
        UBHeap_Free(headerBuffer, 0);
    }
    
    if (cachedDocument != NULL) {
        MmeCache_DeleteDocument(cachedDocument);
    }
    
    /* Convert HRESULT to proper return code */
    if ((resultCode & 0x1FFF0000) == 0x70000) {
        resultCode = resultCode & 0xFFFF;
    }
    
    return (long)resultCode;
}

long Protocol_MBufToFifo(s_URLDocument *document, MBuf *mbuf, int param_3, unsigned int param_4)
{
    unsigned char *dataPtr;
    void *fifoEntry;
    long result;
    int copySize;
    
    if (mbuf == NULL) {
        return 2;  /* Invalid parameter */
    }
    
    dataPtr = mbuf->buf;
    if ((unsigned int)dataPtr < 0x10) {
        return 2;  /* Invalid buffer */
    }
    
    /* Allocate FIFO entry */
    fifoEntry = UBHeap_Alloc(mbuf->length + sizeof(void *) * 3, eHeapUsageMBuf, 0);
    if (fifoEntry == NULL) {
        return -0x7FE9FFFF;  /* Out of memory */
    }
    
    /* Initialize FIFO entry */
    *(int *)((char *)fifoEntry + 8) = 0;  /* Set length to 0 */
    *(MBuf **)((char *)fifoEntry + 4) = mbuf;  /* Store MBuf pointer */
    
    /* Copy data from MBuf to FIFO */
    copySize = MBufC_CopyOut(mbuf, dataPtr, mbuf->length, 0);
    
    /* Update document's body buffer size */
    if (document->body_buffer != NULL) {
        *(int *)document->body_buffer = (int)((char *)document->body_buffer + copySize - 0x10);
    }
    
    /* Add to FIFO list */
    if (document->type_fifo_head == NULL) {
        /* First entry */
        document->type_fifo_head = fifoEntry;
    }
    else {
        /* Append to tail */
        *(void **)document->type_fifo_tail = fifoEntry;
    }
    
    /* Update tail pointer and clear next pointer */
    *(void **)fifoEntry = NULL;
    document->type_fifo_tail = fifoEntry;
    
    /* Set flag */
    document->pending_flags |= 0x1000;
    
    result = 0;  /* Success */
    
    return result;
}

