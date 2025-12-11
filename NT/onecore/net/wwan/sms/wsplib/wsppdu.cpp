#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>

/* Structure definitions */
typedef struct WapAddr {
    unsigned int iLength;
    wchar_t *iAddress;
} WapAddr;

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

typedef struct s_RequestData {
    void *iDoc;
    void *iNextRequest;
    int iUnknown;
} s_RequestData;

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

typedef struct s_SingleLink {
    struct s_SingleLink *iNext;
} s_SingleLink;

typedef struct s_SingleList {
    s_SingleLink *iHead;
    s_SingleLink *iTail;
} s_SingleList;

typedef struct SecurityValues {
    int iProtocol;
    unsigned long iVersion;
    unsigned int iBea;
    unsigned int iBeaBits;
    unsigned int iMac;
    unsigned int iMacBits;
    unsigned int iPublicKey;
    unsigned int iPkBits;
    char *iSubject;
    char *iIssuer;
    long iValidNotBefore;
    long iValidNotAfter;
    int iStatus;
} SecurityValues;

typedef struct s_URLDocument {
    void *base;
    void *request;
    int iOriginTrustLevel;
    int iTrustLevel;
    SecurityValues iSecurity;
    int iDocumentCharset;
    long error;
    unsigned long pending_flags;
    unsigned long flags;
    unsigned long iContentTypeWspToken;
    unsigned int type;
    unsigned int return_code;
    long expires;
    long cache_time;
    unsigned long size;
    unsigned long size_available;
    s_SingleList headers;
    void *iAuthenticationChallenge;
    void *iProxyAuthChallenge;
    void *scheme_ws;
    s_SingleList scheme_fifo;
    int scheme_module_flag;
    void *iSchemePlugin;
    unsigned int state;
    void *socket;
    void *type_ws;
    s_SingleList type_fifo;
    int type_decoder_flag;
    void *iContentPlugin;
    unsigned int type_state;
    int unget_buf;
    s_SingleList body;
} s_URLDocument;

typedef struct s_CacheEntry s_CacheEntry;

typedef enum CacheType {
    kCacheUrlDocument = 0,
    kCacheNamedAtom = 1
} CacheType;

union CacheData {
    void *iDummy;
    struct s_URLDocument *iDocument;
    void *iNamedAtom;
};

struct s_CacheEntry {
    struct s_CacheEntry *iNext;
    long iLastAccess;
    unsigned int iRef;
    CacheType iType;
};

long PDU_DecodeLongInteger(unsigned char *data, int length);
int PDU_ProcessHeaderParameter(s_RequestData *request, unsigned char *data, int length);
int PDU_UintVarLen(unsigned long value);
unsigned long PDU_UintVarRead(unsigned char *data);

/* External functions */
extern "C" MBuf *MBufC_Alloc(int size);
extern "C" void MBufC_Free(MBuf *mbufChain);
extern "C" int MBufC_CopyIn(MBuf *mbufChain, unsigned char *data, int offset, int size);
extern "C" int MBufC_CopyOut(MBuf *mbufChain, unsigned char *buffer, int offset, int size);
long WSP_ProcessPushRequest(MBuf *mbuf, WapAddr *addr, int param3, int param4,
                           unsigned char **dataPtr, unsigned long *sizePtr, 
                           wchar_t **headersPtr);
extern long Header_AddTokenized(s_RequestData *request, unsigned char token, char *headerValue);
extern long Header_AddNamed(s_RequestData *request, char *headerName, char *headerValue);
extern long Protocol_MBufToFifo(s_URLDocument *document, MBuf *mbuf, int param3, unsigned int param4);
s_CacheEntry *gCache_CachedDocuments = {0};
extern "C" void MmeCache_DeleteDocument(s_URLDocument *document);
extern "C" long MmeCache_AddDocument(s_URLDocument *document);
extern "C" long lUrl_Parse(s_ParsedURL **result, char *urlString, int param3, int param4);
extern long Notify_NewDocument(char *url, unsigned char **documentData, unsigned long *dataSize, wchar_t **headers);
extern "C" void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param3);
extern "C" void *UBHeap_AllocZero(unsigned int size, int usage, unsigned int param3);
extern "C" void UBHeap_Free(void *ptr, unsigned int param2);

/* Global variables */
extern wchar_t *WSP_MIME_TYPE[0x4F];
struct {
    wchar_t *pszMimeTypeString;
    unsigned long iTokenValue;
} g_sAdditionalMimeTypes[7] = {
    {L"text/vnd.wap.emn+xml", 0x304},
    {L"application/vnd.wap.emn+wbxml", 0x305},
    {L"application/vnd.wap.emn+wbxml", 0x30A},
    {L"application/vnd.cmcc.setting+wbxml", 0x30E},
    {L"application/vnd.cmcc.bombing+wbxml", 0x30F},
    {L"application/vnd.omaloc-supl-init", 0x312},
    {L"application/oma-supl-ulp", 0x312}
};
extern wchar_t *WSP_APP_ID[0x11];
extern char s_wsp[];

unsigned int DecodeWspHeaders(unsigned char *param1, unsigned long param2, 
                              unsigned char **param3, unsigned long *param4, 
                              wchar_t **param5)
{
    MBuf *mbuf;
    int copyResult;
    unsigned int resultCode;
    void *buffer = NULL;
    wchar_t *temp;
    unsigned int tempResult;
    
    mbuf = MBufC_Alloc(0);
    if (mbuf == NULL) {
        return E_OUTOFMEMORY;  /* E_OUTOFMEMORY */
    }
    
    copyResult = MBufC_CopyIn(mbuf, (unsigned char *)param2, 0, 0);
    if (copyResult == 0) {
        if (buffer == NULL) {
            resultCode = E_OUTOFMEMORY;  /* E_OUTOFMEMORY */
        }
        else {
            /* Set up buffer */
            *(void **)((char *)buffer + 12) = (char *)buffer + 16;
            StringCchCopyW(*(wchar_t **)((char *)buffer + 12), 0x36, L"IncomingWapMessage");
            
            /* Set header values */
            *(int *)buffer = 0x12;
            *(int *)((char *)buffer + 4) = 0;
            *(short *)((char *)buffer + 8) = 0xB84;
            
            /* Process push request */
            tempResult = WSP_ProcessPushRequest(NULL, NULL, (int)param3, (int)param4, 
                                               (unsigned char**)param5, NULL, NULL);
            resultCode = 0;
            
            if (tempResult != 0 && tempResult > 0) {
                resultCode = (tempResult & 0xFFFF) | 0x80070000;
            }
            
        }
    }
    else {
        resultCode = E_FAIL;  /* E_FAIL */
    }
    
    MBufC_Free(mbuf);
    return resultCode;
}

void FreeWspHeaders(void *headers, void *data)
{
    if (headers != NULL) {
    }
    if (data != NULL) {
    }
    return;
}

long PDU_DecodeInteger(unsigned char *data, int *length)
{
    unsigned char firstByte;
    long result;
    
    if (data == NULL) {
        return 0;
    }
    
    firstByte = *data;
    
    if (firstByte > 0x7F) {
        if (length != NULL) {
            *length = 1;
        }
        return firstByte & 0x7F;
    }
    
    if (length != NULL) {
        *length = firstByte;
    }
    
    result = PDU_DecodeLongInteger(data + 1, firstByte);
    return result;
}

long PDU_DecodeLongInteger(unsigned char *data, int length)
{
    unsigned char *current;
    int result = 0;
    
    if (data == NULL || length == 0) {
        return 0;
    }
    
    if (length > 4) {
        return 0x7FFFFFFF;
    }
    
    current = data;
    
    if (length > 3) {
        result = (unsigned int)(*current) << 24;
        current++;
    }
    
    if (length > 2) {
        result += (unsigned int)(*current) << 16;
        current++;
    }
    
    if (length > 1) {
        result += (unsigned int)(*current) << 8;
        current++;
    }
    
    result += *current;
    
    return result;
}

long PDU_ProcessHeader(s_RequestData *request, unsigned char headerType, 
                      unsigned char *data, int dataLength)
{
    unsigned int type = headerType;
    long result = 0;
    long tempResult;
    unsigned char *currentData = data;
    int decodedLength;
    int tempInt;
    char tempBuffer[12];
    
    /* Security cookie */
    unsigned int savedCookie = __security_cookie;
    
    if (type < 0x20) {
        if (type == 0x1F) {
            /* Special handling for type 0x1F */
            if (*(char *)&request->iDoc == 0x80) {
                /* Clear flag */
                ((s_URLDocument *)request->iDoc)->flags &= 0xFFEFFFFF;
            }
        }
        else if (type == 0x08) {
            /* Type 0x08 handling */
            result = 0;
            unsigned char subType = *(unsigned char *)&request->iDoc & 0x7F;
            
            if (subType != 0) {
                if (subType == 2) {
                    s_URLDocument *doc = (s_URLDocument *)request->iDoc;
                    if (doc->expires == 0 || doc->expires == -1) {
                        tempResult = PDU_DecodeInteger(currentData, &decodedLength);
                        doc->expires = tempResult + doc->cache_time;
                    }
                }
                else if (subType == 6) {
                    ((s_URLDocument *)request->iDoc)->flags &= 0xFFDFFFFF;
                }
                else if (subType == 9) {
                    ((s_URLDocument *)request->iDoc)->flags &= 0xFFEFFFFF;
                }
            }
            else {
                ((s_URLDocument *)request->iDoc)->flags &= 0xFFEFFFFF;
            }
        }
        else if (type == 0x0D) {
            /* Content length */
            tempResult = PDU_DecodeLongInteger(currentData, dataLength);
            ((s_URLDocument *)request->iDoc)->size = tempResult;
        }
        else if (type == 0x11) {
            /* Header field */
            int remaining = dataLength;
            if (remaining == 1 || *(char *)&request->iDoc < 0) {
                unsigned int token = *(unsigned char *)&request->iDoc & 0x7F;
                ((s_URLDocument *)request->iDoc)->iContentTypeWspToken = token;
                
                s_URLDocument *doc = (s_URLDocument *)request->iDoc;
                if (doc->iContentPlugin == NULL && doc->base != NULL) {
                    doc->pending_flags &= 0xFFFFFDFF;
                }
                
                while (remaining > 1) {
                    remaining = PDU_ProcessHeaderParameter(request, currentData, remaining);
                }
                
                if (((s_URLDocument *)request->iDoc)->flags & 0x200000) {
                    ((s_URLDocument *)request->iDoc)->iContentPlugin = NULL;
                    ((s_URLDocument *)request->iDoc)->pending_flags &= 0xFFFFFDFF;
                }
            }
            else {
                unsigned char subType = *(unsigned char *)&request->iDoc;
                if (subType > 0x1E) {
                    /* Text header */
                    int textLength = 0;
                    while (subType != 0 && textLength <= remaining) {
                        textLength++;
                        subType = *((unsigned char *)&request->iDoc + textLength);
                    }
                    
                    if (*((char *)&request->iDoc + textLength) == '\0') {
                        Header_AddTokenized(request, *currentData, (char *)currentData);
                    }
                    textLength++;
                    
                    while (remaining > 1) {
                        remaining = PDU_ProcessHeaderParameter(request, currentData, remaining);
                    }
                }
                else {
                    /* Integer header */
                    int intLength = subType + 1;
                    if (intLength <= remaining) {
                        unsigned int value = PDU_DecodeLongInteger(currentData, intLength);
                        if (value != 0x7FFFFFFF) {
                            ((s_URLDocument *)request->iDoc)->iContentTypeWspToken = value;
                            s_URLDocument *doc = (s_URLDocument *)request->iDoc;
                            if (doc->iContentPlugin == NULL && doc->base != NULL) {
                                doc->pending_flags &= 0xFFFFFDFF;
                            }
                            
                            while (remaining > 1) {
                                remaining = PDU_ProcessHeaderParameter(request, currentData, remaining);
                            }
                            
                            if (((s_URLDocument *)request->iDoc)->flags & 0x200000) {
                                ((s_URLDocument *)request->iDoc)->iContentPlugin = NULL;
                                ((s_URLDocument *)request->iDoc)->pending_flags &= 0xFFFFFDFF;
                            }
                        }
                        else {
                            result = -0x7FE9FFFE;
                        }
                    }
                }
            }
        }
        else if (type == 0x14) {
            /* Expires header */
            tempResult = PDU_DecodeLongInteger(currentData, dataLength);
            ((s_URLDocument *)request->iDoc)->expires = tempResult;
        }
    }
    else if (type == 0x2D) {
        result = -0x7FF7FFFE;
    }
    else if (type == 0x2F) {
        /* Application ID header */
        char buffer[12] = {0};
        ((s_URLDocument *)request->iDoc)->scheme_module_flag = 1;
        
        tempResult = PDU_DecodeLongInteger(currentData, 11);
        if (_itoa_s(tempResult, buffer, 12, 10) == 0) {
            Header_AddTokenized(request, *currentData, buffer);
            result = 0;
            ((s_URLDocument *)request->iDoc)->scheme_module_flag = 0;
        }
        else {
            result = -0x7FE9FFFF;
        }
    }
    else if (type == 0x34) {
        /* Unknown header type 0x34 */
        char buffer[12] = {0};
        tempResult = PDU_DecodeLongInteger(currentData, 11);
        if (_itoa_s(tempResult, buffer, 12, 10) == 0) {
            Header_AddTokenized(request, 10, buffer);
        }
        else {
            result = -0x7FE9FFFF;
        }
    }
    
    __security_check_cookie(savedCookie);
    return result;
}

int PDU_ProcessHeaderParameter(s_RequestData *request, unsigned char *data, int length)
{
    unsigned char firstByte = *data;
    s_RequestData *tempRequest = request;
    int remaining = length;
    char buffer[12];
    long decodedValue;
    int result;
    
    /* Security cookie */
    unsigned int savedCookie = __security_cookie;
    
    if (firstByte & 0x80) {
        /* High bit set */
        unsigned char paramType = firstByte & 0x7F;
        data++;
        
        if (paramType == 1) {
            decodedValue = PDU_DecodeInteger(data, &remaining);
            ((s_URLDocument *)request->iDoc)->size_available = decodedValue;
        }
        else if (paramType == 0x11) {
            decodedValue = PDU_DecodeInteger(data, &remaining);
            if (_itoa_s(decodedValue, buffer, 12, 10) == 0) {
                Header_AddNamed(request, buffer, buffer);
            }
            else {
                goto error;
            }
        }
        else if (paramType == 0x12) {
            /* Skip to null terminator */
            while (*data != 0) {
                if (remaining <= 0) break;
                data++;
                remaining--;
            }
            Header_AddNamed(request, (char *)data, (char *)data);
        }
        else {
            /* Skip parameter */
            while (*data != 0 && remaining > 0) {
                data++;
                remaining--;
            }
        }
    }
    else {
        /* Skip to null terminator */
        while (firstByte != 0) {
            if (tempRequest == NULL) goto error;
            data++;
            tempRequest = (s_RequestData *)((char *)tempRequest - 1);
            firstByte = *data;
        }
        
        if (tempRequest == NULL) goto error;
        
        data++;
        firstByte = *data;
        
        if (firstByte & 0x80) {
            /* Convert to string */
            if (_itoa_s(firstByte & 0x7F, buffer, 12, 10) == 0) {
                Header_AddNamed(request, buffer, buffer);
            }
            else {
                goto error;
            }
        }
        else {
            /* Skip to null terminator */
            while (firstByte != 0 && remaining > 0) {
                data++;
                remaining--;
                firstByte = *data;
            }
            Header_AddNamed(request, (char *)data, (char *)data);
        }
    }
    
    result = (int)tempRequest;
    goto cleanup;
    
error:
    result = 0;
    
cleanup:
    __security_check_cookie(savedCookie);
    return result;
}

long PDU_ProcessHeaders(s_RequestData *request, unsigned char *data, int length)
{
    int position = 0;
    int result = 0;
    unsigned char currentByte;
    unsigned int paramLength;
    int lengthBytes;
    
    /* Security cookie */
    unsigned int savedCookie = __security_cookie;
    
    if (length > 0) {
        do {
            if (result != 0) break;
            
            if (position == 0) {
                currentByte = 0x11;
            }
            else {
                currentByte = data[position];
                if (currentByte < 0x80) {
                    /* Skip to null terminator */
                    while (currentByte != 0) {
                        if (position >= length) {
                            result = -0x7FF77FFF;
                            goto cleanup;
                        }
                        position++;
                        currentByte = data[position];
                    }
                    currentByte = 0xFF;
                }
                else {
                    currentByte &= 0x7F;
                }
                position++;
            }
            
            if (position < length) {
                unsigned char lengthByte = data[position];
                
                if (lengthByte < 0x1F) {
                    paramLength = lengthByte;
                    position++;
                    
                    if (length - position < (int)paramLength) {
                        result = -0x7FF77FFF;
                        goto cleanup;
                    }
                    
                    if (paramLength == 0) {
                        result = 0;
                    }
                    else {
                        result = PDU_ProcessHeader(request, currentByte, data + position, paramLength);
                    }
                }
                else if (lengthByte == 0x1F) {
                    paramLength = PDU_UintVarRead(data + position + 1);
                    lengthBytes = PDU_UintVarLen(paramLength);
                    position += 1 + lengthBytes;
                    
                    if (position >= length || length - position < (int)paramLength) {
                        result = -0x7FF77FFF;
                        goto cleanup;
                    }
                    
                    if (paramLength != 0) {
                        result = PDU_ProcessHeader(request, currentByte, data + position, paramLength);
                    }
                    else {
                        result = 0;
                    }
                }
                else {
                    paramLength = 0;
                    if (lengthByte < 0x80) {
                        if (lengthByte == 0x7F) {
                            position++;
                        }
                        
                        int startPos = position;
                        while (data[position] != 0) {
                            if (position >= length) {
                                result = -0x7FF77FFF;
                                goto cleanup;
                            }
                            paramLength++;
                            position++;
                        }
                        
                        if (currentByte == 0xFF) {
                            result = Header_AddNamed(request, (char *)(data + startPos), 
                                                    (char *)(data + startPos));
                        }
                        else {
                            result = Header_AddTokenized(request, currentByte, 
                                                        (char *)(data + startPos));
                        }
                    }
                    else {
                        lengthByte &= 0x7F;
                        result = PDU_ProcessHeader(request, 1, &lengthByte, 1);
                    }
                    position++;
                }
                
                position += paramLength;
                if ((int)paramLength < 0) {
                    result = 2;
                }
            }
            else {
                result = -0x7FF77FFF;
            }
        } while (position < length);
    }
    
cleanup:
    __security_check_cookie(savedCookie);
    return result;
}

int PDU_UintVarLen(unsigned long value)
{
    if (value < 0x80) {
        return 1;
    }
    if (value < 0x4000) {
        return 2;
    }
    if (value < 0x200000) {
        return 3;
    }
    return (value > 0xFFFFFFF) ? 5 : 4;
}

unsigned long PDU_UintVarRead(unsigned char *data)
{
    unsigned long result = 0;
    unsigned char currentByte;
    int byteCount = 0;
    
    if (data == NULL) {
        return 0;
    }
    
    do {
        currentByte = *data;
        data++;
        
        if (byteCount >= 5) {
            return result;
        }
        
        byteCount++;
        result = result * 0x80 + (currentByte & 0x7F);
    } while (currentByte & 0x80);
    
    return result;
}

int UseAdvancedActivation(void)
{
    wchar_t buffer[260];
    DWORD bufferSize = 260;
    DWORD type;
    LONG result;
    int compareResult;
    
    /* Security cookie */
    unsigned int savedCookie = __security_cookie;
    
    result = RegGetValueW(HKEY_LOCAL_MACHINE,
                         L"Software\\OEM\\OMADM",
                         L"ActivationType",
                         RRF_RT_REG_SZ,
                         &type,
                         buffer,
                         &bufferSize);
    
    if (result != ERROR_SUCCESS) {
        result = (result & 0xFFFF) | 0x80070000;
    }
    
    if (result >= 0) {
        compareResult = _wcsicmp(buffer, L"Sprint");
        result = (compareResult == 0) ? 1 : 0;
    }
    
    __security_check_cookie(savedCookie);
    return (int)result;
}

long Wap_CreateStringFromAddr(WapAddr *addr, int param2, char **output, int param4)
{
    int wideLen;
    void *buffer;
    int multiByteLen;
    long result = 0;
    
    if (addr == NULL || output == NULL || addr->iLength == 0) {
        return -0x7FF8FFA9;
    }
    
    /* Get required buffer size */
    wideLen = WideCharToMultiByte(CP_ACP, 0, addr->iAddress, -1, NULL, 0, NULL, NULL);
    
    if (wideLen > 0) {
        buffer = UBHeap_Alloc(wideLen, 3, 0);  /* eHeapUsageString = 3 */
        if (buffer == NULL) {
            return -0x7FF8FFF2;
        }
        
        /* Convert to multi-byte */
        multiByteLen = WideCharToMultiByte(CP_ACP, 0, addr->iAddress, -1, 
                                          (char *)buffer, wideLen, NULL, NULL);
        
        if (multiByteLen > 0) {
            *output = (char *)buffer;
            addr->iLength = wideLen;
        }
        else {
            UBHeap_Free(buffer, 0);
            result = -0x7FFFBFFB;
        }
    }
    else {
        result = -0x7FFFBFFB;
    }
    
    if (result < 0) {
        result = -0x7FE97FFF;
    }
    
    return result;
}

long WSP_DecodeResponsePDU(s_RequestData *request, MBuf *mbuf, int param3, int param4)
{
    unsigned char *dataPtr = NULL;
    MBuf *currentMbuf = mbuf;
    unsigned int totalLength = 0;
    long result = 0;
    int advancedActivation;
    unsigned char *tempPtr;
    unsigned int pduLength;
    int lengthBytes;
    void *buffer;
    unsigned int bufferSize;
    int headerResult;
    int chunkSize;
    int remaining;
    
    /* Initialize */
    ((s_URLDocument *)request->iDoc)->scheme_module_flag = 0;
    
    /* Calculate total length of MBuf chain */
    while (currentMbuf != NULL) {
        totalLength += currentMbuf->length;
        currentMbuf = currentMbuf->next;
    }
    
    if (totalLength != 0) {
        /* Copy data to temporary buffer */
        unsigned char *tempBuffer = (unsigned char *)malloc(totalLength < 12 ? totalLength : 12);
        if (tempBuffer == NULL) {
            return -0x7FF7FFFF;
        }
        
        MBufC_CopyOut(mbuf, tempBuffer, 0, totalLength < 12 ? totalLength : 12);
        
        advancedActivation = UseAdvancedActivation();
        
        if (advancedActivation == 0) {
            goto process_headers;
        }
        
        if (totalLength < 6) {
            result = -0x7FF77FFF;
            goto cleanup;
        }
        
        if (tempBuffer[0] == 0x06) {
            /* Parse PDU length */
            pduLength = PDU_UintVarRead(tempBuffer + 1);
            lengthBytes = PDU_UintVarLen(pduLength);
            
            if (pduLength - 1 > 0xFFF) {
                goto process_headers;
            }
            
            if (totalLength < (unsigned int)(6 + lengthBytes + pduLength)) {
                result = -0x7FF77FFF;
                goto cleanup;
            }
            
            /* Allocate buffer for PDU */
            bufferSize = pduLength;
            buffer = UBHeap_Alloc(bufferSize, 7, 0);  /* eHeapUsageMBuf = 7 */
            if (buffer == NULL) {
                result = -0x7FF7FFFF;
                goto cleanup;
            }
            
            /* Copy PDU data */
            MBufC_CopyOut(mbuf, (unsigned char *)buffer, 6 + lengthBytes, bufferSize);
            
            /* Process headers */
            if (((unsigned char *)buffer)[0] == 0xC4) {
                headerResult = PDU_ProcessHeaders(request, (unsigned char *)buffer, bufferSize);
            }
            else {
                headerResult = PDU_ProcessHeaders(request, (unsigned char *)buffer, bufferSize);
            }
            
            UBHeap_Free(buffer, 0);
            
            if (headerResult == 0 && ((s_URLDocument *)request->iDoc)->iContentTypeWspToken == 0x44) {
                /* Success with special condition */
                result = 0;
            }
            else {
                result = headerResult;
            }
            
            goto cleanup;
        }
        
process_headers:
        /* Process remaining data */
        remaining = totalLength - 1;
        if (remaining > 0) {
            pduLength = PDU_UintVarRead(tempBuffer + 1);
            lengthBytes = PDU_UintVarLen(pduLength);
            
            if (1 + lengthBytes + pduLength <= totalLength) {
                if (pduLength == 0) {
                    result = 0;
                    goto cleanup;
                }
                
                bufferSize = pduLength;
                buffer = UBHeap_Alloc(bufferSize, 7, 0);
                if (buffer == NULL) {
                    result = -0x7FF7FFFF;
                    goto cleanup;
                }
                
                /* Copy and process PDU */
                MBufC_CopyOut(mbuf, (unsigned char *)buffer, 1 + lengthBytes, bufferSize);
                result = PDU_ProcessHeaders(request, (unsigned char *)buffer, bufferSize);
                
                if (result == 0 && !(((s_URLDocument *)request->iDoc)->flags & 0x80000000)) {
                    /* Process remaining data in chunks */
                    remaining = totalLength - (1 + lengthBytes + pduLength);
                    
                    if (remaining > 0xF0) {
                        int numChunks = (remaining + 0xEF) / 0xF0;
                        remaining -= numChunks * 0xF0;
                        
                        for (int i = 0; i < numChunks; i++) {
                            result = Protocol_MBufToFifo((s_URLDocument *)request->iDoc, 
                                                         mbuf, 0xF0, 0);
                            if (result != 0) {
                                break;
                            }
                        }
                        
                        if (result != 0) {
                            UBHeap_Free(buffer, 0);
                            goto cleanup;
                        }
                    }
                    
                    if (remaining > 0) {
                        if (remaining <= 0xF0) {
                            result = Protocol_MBufToFifo((s_URLDocument *)request->iDoc, 
                                                         mbuf, remaining, 0);
                        }
                        else {
                            result = 2;
                        }
                    }
                }
                
                UBHeap_Free(buffer, 0);
            }
            else {
                result = -0x7FF77FFF;
            }
        }
        
cleanup:
        free(tempBuffer);
    }
    
    return result;
}

long WSP_ProcessPushRequest(MBuf *mbuf, WapAddr *addr, int param3, int param4,
                           unsigned char **dataPtr, unsigned long *sizePtr, 
                           wchar_t **headersPtr)
{
    char *addressStr = NULL;
    long result;
    s_URLDocument *document;
    char *buffer;
    int bufferLen;
    char *temp;
    int remaining;
    int parseResult;
    s_CacheEntry *cacheEntry;
    
    /* Convert address to string */
    result = Wap_CreateStringFromAddr(addr, 0, &addressStr, 0);
    if (result != 0) {
        return result;
    }
    
    /* Allocate document */
    document = (s_URLDocument *)UBHeap_AllocZero(sizeof(s_URLDocument), 6, 0);  /* eHeapUsageURL = 6 */
    if (document == NULL) {
        UBHeap_Free(addressStr, 0);
        return -0x7FE9FFFF;
    }
    
    /* Add to cache */
    result = MmeCache_AddDocument(document);
    if (result != 0) {
        UBHeap_Free(document, 0);
        UBHeap_Free(addressStr, 0);
        return result;
    }
    
    /* Create buffer for URL */
    bufferLen = strlen(addressStr) + 0x1D;
    buffer = (char *)UBHeap_Alloc(bufferLen, 6, 0);
    if (buffer == NULL) {
        cacheEntry = gCache_CachedDocuments;
        while (cacheEntry != NULL) {
            if ((s_URLDocument *)((char *)cacheEntry + 0x10) == document) { /* Offset to iData */
                cacheEntry->iRef--;
                break;
            }
            cacheEntry = cacheEntry->iNext;
        }
        
        MmeCache_DeleteDocument(document);
        UBHeap_Free(document, 0);
        UBHeap_Free(addressStr, 0);
        return -0x7FE9FFFF;
    }
    
    /* Build URL */
    StringCchCopyA(buffer, bufferLen, addressStr);
    StringCchCatA(buffer, bufferLen, "/0x");
    
    /* Calculate remaining space */
    temp = buffer;
    while (*temp != '\0') {
        temp++;
    }
    remaining = bufferLen - (temp - buffer);
    
    /* Append parameter */
    if (_itoa_s((int)document, temp, remaining, 16) != 0) {
        UBHeap_Free(buffer, 0);
        cacheEntry = gCache_CachedDocuments;
        while (cacheEntry != NULL) {
            if ((s_URLDocument *)((char *)cacheEntry + 0x10) == document) { /* Offset to iData */
                cacheEntry->iRef--;
                break;
            }
            cacheEntry = cacheEntry->iNext;
        }
        
        MmeCache_DeleteDocument(document);
        UBHeap_Free(document, 0);
        UBHeap_Free(addressStr, 0);
        return -0x7FE9FFFF;
    }
    
    /* Parse URL */
    parseResult = lUrl_Parse((s_ParsedURL **)&document->request, buffer, 0, 0);
    document->base = document->request;
    document->flags |= 0x400000;
    
    if (parseResult == 0) {
        /* Decode response */
        result = WSP_DecodeResponsePDU((s_RequestData *)document->request, mbuf, 0, 0);
        
        if (result == 0) {
            document->scheme_module_flag = 1;
            document->flags |= 0x180000;
            
            /* Notify new document */
            result = Notify_NewDocument((char *)param4, dataPtr, (unsigned long *)document->request, 
                                       headersPtr);
        }
    }
    else {
        result = parseResult;
    }
    
    /* Clean up cache reference */
    cacheEntry = gCache_CachedDocuments;
    while (cacheEntry != NULL) {
        if ((s_URLDocument *)((char *)cacheEntry + 0x10) == document) { /* Offset to iData */
            cacheEntry->iRef--;
            break;
        }
        cacheEntry = cacheEntry->iNext;
    }
    
    if (result != 0) {
        MmeCache_DeleteDocument(document);
        UBHeap_Free(document, 0);
    }
    
    UBHeap_Free(buffer, 0);
    UBHeap_Free(addressStr, 0);
    
    return result;
}

