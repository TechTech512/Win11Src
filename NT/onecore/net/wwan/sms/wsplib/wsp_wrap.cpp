#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Structure definitions */
typedef struct s_URLDocument {
    void *base;
    void *request;
    int iOriginTrustLevel;
    int iTrustLevel;
    /* ... other members ... */
    void *body_buffer;   /* Offset 0x6c */
    void *type_fifo_head; /* Offset 0xa0 */
    /* ... other members ... */
} s_URLDocument;

typedef struct s_SingleLink {
    struct s_SingleLink *iNext;
} s_SingleLink;

typedef struct s_SingleList {
    s_SingleLink *iHead;
    s_SingleLink *iTail;
} s_SingleList;

typedef struct MIMETypeEntry {
    wchar_t *pszMimeTypeString;
    unsigned long iTokenValue;
} MIMETypeEntry;

/* External functions */
extern "C" void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern "C" void UBHeap_Free(void *ptr, unsigned int param_2);
extern "C" s_SingleLink *SingleListRemoveHead(s_SingleList *list);

/* Global variables */
wchar_t *WSP_MIME_TYPE[80] = {
    L"*/*",
    L"text/*",
    L"text/html",
    L"text/plain",
    L"text/x-hdml",
    L"text/x-ttml",
    L"text/x-vCalendar",
    L"text/x-vCard",
    L"text/vnd.wap.wml",
    L"text/vnd.wap.wmlscript",
    L"text/vnd.wap.channel",
    L"Multipart/*",
    L"Multipart/mixed",
    L"Multipart/form-data",
    L"Multipart/byteranges",
    L"multipart/alternative",
    L"application/*",
    L"application/java-vm",
    L"application/x-www-form-urlencoded",
    L"application/x-hdmlc",
    L"application/vnd.wap.wbxml",
    L"application/vnd.wap.wmlc",
    L"application/vnd.wap.wmlscriptc",
    L"application/vnd.wap.sic",
    L"application/vnd.wap.slc",
    L"application/vnd.wap.coc",
    L"application/x-x509-ca-cert",
    L"application/x-x509-user-cert",
    L"image/*",
    L"image/gif",
    L"image/jpeg",
    L"image/tiff",
    L"image/png",
    L"image/vnd.wap.wbmp",
    L"application/vnd.wap.multipart.*",
    L"application/vnd.wap.multipart.mixed",
    L"application/vnd.wap.multipart.form-data",
    L"application/vnd.wap.multipart.byteranges",
    L"application/vnd.wap.multipart.alternative",
    L"application/xml",
    L"text/xml",
    L"application/vnd.wap.wbxml",
    L"application/x-x968-cross-cert",
    L"application/x-x968-ca-cert",
    L"application/x-x968-user-cert",
    L"text/vnd.wap.si",
    L"application/vnd.wap.sic",
    L"text/vnd.wap.sl",
    L"application/vnd.wap.slc",
    L"text/vnd.wap.co",
    L"application/vnd.wap.coc",
    L"application/vnd.wap.connectivity-wbxml",
    L"application/vnd.wap.connectivity-xml",
    L"text/vnd.wap.connectivity-text",
    L"application/vnd.wap.connectivity-wbxml",
    L"application/pkcs7-mime",
    L"application/vnd.wap.hashed-certificate",
    L"application/vnd.wap.signed-certificate",
    L"application/vnd.wap.cert-response",
    L"application/xhtml+xml",
    L"application/wml+xml",
    L"text/css",
    L"application/vnd.wap.mms-message",
    L"application/vnd.wap.rollover-certificate",
    L"application/vnd.wap.loi-certificate",
    L"application/vnd.wap.missing-certificate",
    L"application/vnd.syncml.ds.notification",
    L"application/vnd.syncml.ds.changelog",
    L"application/vnd.syncml.dmddf+xml",
    L"application/vnd.wap.wtls-ca-certificate",
    L"application/vnd.wv.csp.cir",
    L"application/vnd.oma.drm.content",
    L"application/vnd.oma.drm.message",
    L"application/vnd.oma.drm.rights+xml",
    L"application/vnd.oma.drm.rights+wbxml",
    L"application/vnd.oma.dd+xml",
    L"application/vnd.wv.csp+xml",
    L"application/vnd.wv.csp+wbxml",
    L"application/vnd.syncml.dm+wbxml",
    NULL
};
extern struct {
    wchar_t *pszMimeTypeString;
    unsigned long iTokenValue;
} g_sAdditionalMimeTypes[7];

unsigned int __stdcall ExtractDocumentData(s_URLDocument *result, unsigned char **dataBuffer, unsigned long *dataSize)
{
    unsigned int bufferSize;
    void *allocatedBuffer;
    unsigned int resultCode;
    unsigned int totalCopied = 0;
    s_SingleLink *currentEntry;
    void *fifoHead;
    
    if (result == NULL || dataBuffer == NULL || dataSize == NULL) {
        return E_INVALIDARG;  /* E_INVALIDARG */
    }
    
    *dataBuffer = NULL;
    result->base = NULL;
    
    /* Get size from document's body buffer */
    if (result->body_buffer != NULL) {
        bufferSize = *(unsigned int *)result->body_buffer;
    }
    else {
        bufferSize = 0;
    }
    
    if (bufferSize != 0) {
        /* Allocate buffer for extracted data */
        allocatedBuffer = UBHeap_Alloc(bufferSize, 7, 0);  /* eHeapUsageMBuf = 7 */
        if (allocatedBuffer == NULL) {
            return E_OUTOFMEMORY;  /* E_OUTOFMEMORY */
        }
        
        /* Process FIFO list */
        fifoHead = result->type_fifo_head;
        while (fifoHead != NULL) {
            /* Remove entry from FIFO */
            currentEntry = SingleListRemoveHead((s_SingleList *)result->type_fifo_head);
            
            /* Copy data from FIFO entry */
            if (currentEntry != NULL) {
                /* Calculate source and destination pointers */
                unsigned char *srcData = (unsigned char *)((char *)currentEntry + 12);  /* Skip header */
                unsigned int entrySize = *(unsigned int *)((char *)currentEntry + 8);  /* Get size */
                
                /* Copy data */
                if (totalCopied + entrySize <= bufferSize) {
                    memcpy((unsigned char *)allocatedBuffer + totalCopied, srcData, entrySize);
                    totalCopied += entrySize;
                }
                
                /* Free FIFO entry */
                UBHeap_Free(currentEntry, 0);
            }
            
            /* Get next entry */
            fifoHead = result->type_fifo_head;
        }
        
        /* Set results */
        result->base = (void *)totalCopied;  /* Store total size in base field */
        *dataBuffer = (unsigned char *)allocatedBuffer;
        if (dataSize != NULL) {
            *dataSize = totalCopied;
        }
        
        resultCode = S_OK;  /* S_OK */
    }
    else {
        resultCode = S_OK;  /* S_OK (no data) */
    }
    
    return resultCode;
}

unsigned long GetMIMEToken(char *mimeType)
{
    wchar_t wideBuffer[260];
    wchar_t *semicolonPos;
    unsigned long tokenValue = 0;
    unsigned long defaultToken = 0;
    unsigned int i;
    int compareResult;
    
    /* Convert MIME type to wide char */
    MultiByteToWideChar(CP_ACP, 0, mimeType, -1, wideBuffer, 260);
    
    /* Strip parameters (everything after semicolon) */
    semicolonPos = (wchar_t*)wcschr(wideBuffer, L';');
    if (semicolonPos != NULL) {
        *semicolonPos = L'\0';
    }
    
    /* Check additional MIME types first */
    for (i = 0; i < 7; i++) {
        if (g_sAdditionalMimeTypes[i].pszMimeTypeString != NULL) {
            compareResult = _wcsicmp(wideBuffer, g_sAdditionalMimeTypes[i].pszMimeTypeString);
            if (compareResult == 0) {
                tokenValue = g_sAdditionalMimeTypes[i].iTokenValue;
                if (tokenValue != 0) {
                    goto cleanup;
                }
                break;
            }
        }
    }
    
    /* Check standard WSP MIME types */
    for (i = 0; i < 0x4F; i++) {  /* 79 types */
        if (WSP_MIME_TYPE[i] != NULL) {
            compareResult = _wcsicmp(wideBuffer, WSP_MIME_TYPE[i]);
            if (compareResult == 0) {
                tokenValue = i;  /* Token is the index */
                break;
            }
        }
    }
    
cleanup:
    return tokenValue;
}

unsigned int __stdcall WSPGetMIMEType(unsigned long token, wchar_t *outputBuffer)
{
    unsigned int result;
    unsigned int i;
    
    result = E_INVALIDARG;  /* E_INVALIDARG - default error */
    
    if (token < 0x4F) {  /* 79 standard types */
        if (outputBuffer != NULL && WSP_MIME_TYPE[token] != NULL) {
            result = StringCchCopyW(outputBuffer, 260, WSP_MIME_TYPE[token]);
        }
        else if (outputBuffer != NULL) {
            /* Buffer is valid but token not found */
            outputBuffer[0] = L'\0';
            result = S_OK;  /* S_OK with empty string */
        }
    }
    else {
        /* Check additional MIME types */
        for (i = 0; i < 7; i++) {
            if (token == g_sAdditionalMimeTypes[i].iTokenValue) {
                if (outputBuffer != NULL && g_sAdditionalMimeTypes[i].pszMimeTypeString != NULL) {
                    result = StringCchCopyW(outputBuffer, 260, g_sAdditionalMimeTypes[i].pszMimeTypeString);
                }
                else if (outputBuffer != NULL) {
                    outputBuffer[0] = L'\0';
                    result = S_OK;  /* S_OK with empty string */
                }
                break;
            }
        }
    }
    
    return result;
}

