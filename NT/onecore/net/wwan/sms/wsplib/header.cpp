#include <stdlib.h>
#include <string.h>

/* Structure definitions */
typedef struct s_RequestData {
    void *iDoc;
    int iIsToken;
    int iThroughProxy;
    int iConnectionClose;
    int iNoContent;
    int iTransferEncoding;
    int iIsSecure;
    /* ... other members ... */
} s_RequestData;

typedef struct s_SingleLink {
    struct s_SingleLink *iNext;
} s_SingleLink;

typedef struct s_SingleList {
    s_SingleLink *iHead;
    s_SingleLink *iTail;
} s_SingleList;

typedef struct s_URLDocument {
    void *base;
    void *request;
    int iOriginTrustLevel;
    int iTrustLevel;
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

typedef struct s_ParsedURL {
    char *scheme;
    /* ... other members ... */
} s_ParsedURL;

typedef enum t_LngIanaCharset {
    eLngCharsetUnknown = 0,
    eLngCharsetUsAscii = 3,
    eLngCharsetIso8859_1 = 4,
    eLngCharsetIso8859_2 = 5,
    eLngCharsetIso8859_3 = 6,
    eLngCharsetIso8859_4 = 7,
    eLngCharsetIso8859_5 = 8,
    eLngCharsetIso8859_6 = 9,
    eLngCharsetIso8859_7 = 10,
    eLngCharsetIso8859_8 = 11,
    eLngCharsetIso8859_9 = 12,
    eLngCharsetShiftJis = 17,
    eLngCharsetUtf8 = 106,
    eLngCharsetUnicode = 1000,
    eLngCharsetGB2312 = 2025,
    eLngCharsetBig5 = 2026,
    eLngCharsetWindows1251 = 2251,
    eLngCharsetUnicodeBigEndian = 32766,
    eLngCharsetUnicodePlatform = 32767,
    eLngCharsetEnd = 32768
} t_LngIanaCharset;

typedef struct HeaderTableEntry {
    char *iName;
    unsigned char iToken;
    long (*iFunction)(s_RequestData *, char *);
} HeaderTableEntry;

long URL_HeaderParseParameters(s_URLDocument *document, char *headerValue);
long URL_AddGenericHeader(s_SingleList *headerList, char *headerName, char *headerValue);
long url_AddGenericHeader(s_SingleList *headerList, char *headerName, unsigned int nameLength, 
                         char *headerValue, unsigned int valueLength);
long process_application_id(s_RequestData *request, char *headerValue);
long process_cache_control(s_RequestData *request, char *headerValue);
long process_connection(s_RequestData *request, char *headerValue);
long process_content_base(s_RequestData *request, char *headerValue);
long process_content_length(s_RequestData *request, char *headerValue);
long process_content_type(s_RequestData *request, char *headerValue);
long process_expires(s_RequestData *request, char *headerValue);
long process_location(s_RequestData *request, char *headerValue);
long process_pragma(s_RequestData *request, char *headerValue);
long process_proxyconn(s_RequestData *request, char *headerValue);
long process_transfer_encoding(s_RequestData *request, char *headerValue);

/* External functions */
extern "C" void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern "C" void UBHeap_Free(void *ptr, unsigned int param_2);
extern "C" long lUrl_Parse(s_ParsedURL **result, char *urlString, int param_3, int param_4);
extern "C" long url_parse_partial(s_ParsedURL **result, s_ParsedURL *baseUrl, char *relativeUrl);
extern long Rtc_ProcessInetDate(char *dateStr);
extern "C" long LngFindIanaFromMime(t_LngIanaCharset *result, char *mimeCharset, unsigned int param_3);
extern unsigned long GetMIMEToken(char *mimeType);

/* Global variables */
HeaderTableEntry gHeaderTable[] = {
    {"Cache-Control", 0x08, process_cache_control},
    {"Connection", 0x09, process_connection},
    {"Content-Base", 0x0A, process_content_base},
    {"Content-Length", 0x0D, process_content_length},
    {"Content-Type", 0x11, process_content_type},
    {"Expires", 0x14, process_expires},
    {"Location", 0x1C, process_location},
    {"Pragma", 0x1F, process_pragma},
    {"Proxy-Authenticate", 0x20, NULL},
    {"Proxy-Connection", 0xFF, process_proxyconn},
    {"Push-Flag", 0x34, NULL},
    {"Set-cookie", 0x43, NULL},
    {"Transfer-Encoding", 0x27, process_transfer_encoding},
    {"WWW-Authenticate", 0x2D, NULL},
    {"X-Wap-Application-Id", 0x2F, process_application_id},
    {"X-Wap-Content-URI", 0x30, NULL},
    {"X-Wap-Initiator-URI", 0x31, NULL},
    {NULL, 0, NULL}  /* Terminator */
};
char *WSP_APP_ID[0x11] = {
    "x-wap-application:*",
    "x-wap-application:push",
    "x-wap-application:wml",
    "x-wap-application:wtai",
    "x-wap-application:mms",
    "x-wap-application:push.sia",
    "x-wap-application:loc",
    "x-wap-application:syncml",
    "x-wap-application:drm",
    "x-wap-application:emn",
    "x-wap-application:wv",
    "",
    "",
    "",
    "",
    "",
    "x-oma-application:ulp"
};
char s_wsp[] = "wsp";

int get_header_index(char *headerName)
{
    unsigned char firstChar;
    unsigned char tableChar;
    int index = 0;
    int compareResult;
    
    if (headerName == NULL || *headerName == '\0') {
        return -1;
    }
    
    /* Convert first character to lowercase for comparison */
    firstChar = (unsigned char)*headerName;
    if (firstChar >= 'A' && firstChar <= 'Z') {
        firstChar = firstChar | 0x20;  /* Convert to lowercase */
    }
    
    /* Special case for 'c' (common headers start with C) */
    if (firstChar != 'c') {
        index = 5;  /* Skip common C headers */
    }
    
    /* Search through header table */
    while (gHeaderTable[index].iName != NULL) {
        tableChar = (unsigned char)*gHeaderTable[index].iName;
        
        /* Convert table character to lowercase */
        if (tableChar >= 'A' && tableChar <= 'Z') {
            tableChar = tableChar | 0x20;
        }
        
        /* Early exit if we've passed where this header should be */
        if (firstChar < tableChar) {
            return -1;
        }
        
        /* Compare header names case-insensitively */
        compareResult = _stricmp(gHeaderTable[index].iName, headerName);
        if (compareResult == 0) {
            return index;
        }
        
        index++;
    }
    
    return -1;
}

long Header_AddNamed(s_RequestData *request, char *headerName, char *headerValue)
{
    int headerIndex;
    long (*handlerFunc)(s_RequestData *, char *);
    long result;
    
    /* Find header in table */
    headerIndex = get_header_index(headerName);
    
    if (headerIndex == -1 || gHeaderTable[headerIndex].iFunction == NULL) {
        /* Generic header handling */
        if ((((s_URLDocument *)request->iDoc)->flags & 0x400000) == 0) {
            result = 0;  /* No special handling needed */
        }
        else {
            result = URL_AddGenericHeader((s_SingleList *)request, headerName, headerValue);
        }
    }
    else {
        /* Call specific handler function */
        handlerFunc = gHeaderTable[headerIndex].iFunction;
        result = (*handlerFunc)(request, headerValue);
    }
    
    return result;
}

long Header_AddTokenized(s_RequestData *request, unsigned char token, char *headerValue)
{
    int headerIndex = 0;
    long (*handlerFunc)(s_RequestData *, char *);
    long result;
    
    /* Find header by token value */
    if (gHeaderTable[0].iToken != 0) {
        do {
            if (gHeaderTable[headerIndex].iToken == token) {
                goto found_header;
            }
            headerIndex++;
        } while (gHeaderTable[headerIndex].iToken != 0);
    }
    headerIndex = -1;
    
found_header:
    if (headerIndex != -1) {
        handlerFunc = gHeaderTable[headerIndex].iFunction;
        if (handlerFunc != NULL) {
            result = (*handlerFunc)(request, headerValue);
            goto cleanup;
        }
        
        /* Fall back to generic header if flag is set */
        if ((((s_URLDocument *)request->iDoc)->flags & 0x400000) != 0) {
            result = URL_AddGenericHeader((s_SingleList *)request, headerValue, 
                                         (char *)(__security_cookie ^ (unsigned int)&request));
            goto cleanup;
        }
    }
    
    result = 0;
    
cleanup:
    return result;
}

unsigned int lGetNum(char *str)
{
    unsigned int result = 0;
    char ch;
    
    if (str == NULL) {
        return 0;
    }
    
    /* Skip whitespace and '=' */
    while (*str == ' ' || (*str >= '\t' && *str <= '\r') || *str == '=') {
        str++;
    }
    
    /* Parse decimal number */
    while ((ch = *str) >= '0' && ch <= '9') {
        result = result * 10 + (ch - '0');
        str++;
    }
    
    return result;
}

long process_application_id(s_RequestData *request, char *headerValue)
{
    unsigned int appId;
    
    if (request->iIsToken != 0) {
        appId = atoi(headerValue);
        if (appId < 0x11) {  /* 17 application IDs */
            headerValue = WSP_APP_ID[appId];
        }
    }
    
    URL_AddGenericHeader((s_SingleList *)headerValue, headerValue, (char *)request);
    return 0;
}

long process_cache_control(s_RequestData *request, char *headerValue)
{
    s_URLDocument *document;
    int compareResult;
    unsigned int maxAge;
    
    document = (s_URLDocument*)request->iDoc;
    
    /* Check for "No-Cache" */
    compareResult = _strnicmp("No-Cache", headerValue, 9);
    if (compareResult == 0) {
        document->flags = document->flags & 0xFFEFFFFF;
        return 0;
    }
    
    /* Check for "Max-Age" */
    char *temp = headerValue;
    while (*temp != '\0') {
        temp++;
    }
    int headerLength = (int)(temp - headerValue);
    
    compareResult = _strnicmp("Max-Age", headerValue, headerLength);
    if (compareResult == 0 && document->expires == 0) {
        maxAge = lGetNum(headerValue);
        document->expires = maxAge + document->cache_time;
    }
    
    return 0;
}

long process_connection(s_RequestData *request, char *headerValue)
{
    if (request->iThroughProxy == 0) {
        if (_stricmp(headerValue, "Close") == 0) {
            request->iConnectionClose = 1;
        }
        else if (_stricmp(headerValue, "Keep-alive") == 0) {
            request->iConnectionClose = 0;
        }
    }
    
    return 0;
}

long process_content_base(s_RequestData *request, char *headerValue)
{
    s_ParsedURL *parsedUrl = NULL;
    long result;
    
    result = lUrl_Parse(&parsedUrl, headerValue, 0, 0);
    if (result == 0) {
        /* Free old base URL if different from request URL */
        if (((s_URLDocument *)request->iDoc)->base != ((s_URLDocument *)request->iDoc)->request) {
            UBHeap_Free(((s_URLDocument *)request->iDoc)->base, 0);
        }
        
        /* Set new base URL */
        ((s_URLDocument *)request->iDoc)->base = parsedUrl;
    }
    
    return result;
}

long process_content_length(s_RequestData *request, char *headerValue)
{
    s_URLDocument *document;
    unsigned int contentLength;
    
    document = (s_URLDocument*)request->iDoc;
    
    if (document->size == 0) {
        contentLength = lGetNum(headerValue);
        document->size = contentLength;
        
        if (document->size == 0) {
            request->iNoContent = 1;
        }
    }
    
    return 0;
}

long process_content_type(s_RequestData *request, char *headerValue)
{
    s_URLDocument *document;
    unsigned long mimeToken;
    long result;
    
    document = (s_URLDocument*)request->iDoc;
    
    /* Get MIME token for content type */
    mimeToken = GetMIMEToken(headerValue);
    document->iContentTypeWspToken = mimeToken;
    
    /* Reset content plugin if flag is set */
    if ((document->flags & 0x200000) != 0) {
        document->iContentPlugin = NULL;
        document->pending_flags = document->pending_flags & 0xFFFFFDFF;
    }
    
    /* Parse additional parameters (e.g., charset) */
    result = URL_HeaderParseParameters(document, headerValue);
    
    return result;
}

long process_expires(s_RequestData *request, char *headerValue)
{
    s_URLDocument *document;
    long expiresTime;
    
    document = (s_URLDocument*)request->iDoc;
    
    expiresTime = Rtc_ProcessInetDate(headerValue);
    document->expires = expiresTime;
    
    if (expiresTime == -1) {
        document->expires = document->cache_time;
    }
    
    return 0;
}

long process_location(s_RequestData *request, char *headerValue)
{
    s_URLDocument *document;
    s_ParsedURL **requestUrlPtr;
    int schemeCompare;
    long result;
    int secureCompare;
    
    document = (s_URLDocument*)request->iDoc;
	
    /* Check if base scheme is WSP */
    schemeCompare = _strnicmp(((s_ParsedURL *)document->base)->scheme, s_wsp, 3);
    
    requestUrlPtr = (s_ParsedURL**)&document->request;
    
    /* Free old base URL if different from request */
    if (document->base != *requestUrlPtr) {
        UBHeap_Free(document->base, 0);
        document->base = *requestUrlPtr;
    }
    
    /* Parse location as relative URL */
    result = url_parse_partial((s_ParsedURL **)headerValue, (s_ParsedURL*)document->base, headerValue);
    
    if (result == 0) {
        /* Success - update document */
        UBHeap_Free(headerValue, 0);
        document->base = *requestUrlPtr;
        
        /* Check if scheme is HTTPS */
        secureCompare = _stricmp((*requestUrlPtr)->scheme, "https");
        request->iIsSecure = (secureCompare == 0);
        
        /* Convert WSP to HTTP if needed */
        if (schemeCompare == 0) {
            schemeCompare = _strnicmp(((s_ParsedURL *)document->base)->scheme, "http", 4);
            if (schemeCompare == 0) {
                /* Convert "http" to "wsp" */
                char *scheme = ((s_ParsedURL *)document->base)->scheme;
                scheme[0] = 'w';
                scheme[1] = 's';
                scheme[2] = 'p';
                scheme[3] = '\0';
            }
        }
        
        result = 0;
    }
    else {
        /* Failed - restore original request URL */
        *requestUrlPtr = (s_ParsedURL*)document->base;
    }
    
    return result;
}

long process_pragma(s_RequestData *request, char *headerValue)
{
    char *temp;
    int compareResult;
    
    temp = headerValue;
    while (*temp != '\0') {
        temp++;
    }
    int headerLength = (int)(temp - headerValue);
    
    compareResult = _strnicmp("No-Cache", headerValue, headerLength);
    if (compareResult == 0) {
        ((s_URLDocument *)request->iDoc)->flags = ((s_URLDocument *)request->iDoc)->flags & 0xFFEFFFFF;
    }
    else {
        URL_AddGenericHeader((s_SingleList *)headerValue, headerValue, (char *)request);
    }
    
    return 0;
}

long process_proxyconn(s_RequestData *request, char *headerValue)
{
    if (request->iThroughProxy != 0) {
        if (_stricmp(headerValue, "Close") == 0) {
            request->iConnectionClose = 1;
        }
        else if (_stricmp(headerValue, "Keep-alive") == 0) {
            request->iConnectionClose = 0;
        }
    }
    
    return 0;
}

long process_transfer_encoding(s_RequestData *request, char *headerValue)
{
    if (_stricmp(headerValue, "chunked") == 0) {
        request->iTransferEncoding = 1;
    }
    
    return 0;
}

long URL_AddGenericHeader(s_SingleList *headerList, char *headerName, char *headerValue)
{
    char *nameEnd;
    char *valueEnd;
    long result;
    
    if (headerName == NULL) {
        return -1;
    }
    
    /* Calculate header name length */
    nameEnd = headerName;
    while (*nameEnd != '\0') {
        nameEnd++;
    }
    int nameLength = (int)(nameEnd - headerName);
    
    /* Calculate header value length */
    valueEnd = headerValue;
    if (valueEnd != NULL) {
        while (*valueEnd != '\0') {
            valueEnd++;
        }
        int valueLength = (int)(valueEnd - headerValue);
        
        result = url_AddGenericHeader((s_SingleList *)headerList, headerName, nameLength, 
                                      headerValue, valueLength);
    }
    else {
        result = url_AddGenericHeader((s_SingleList *)headerList, headerName, nameLength, 
                                      NULL, 0);
    }
    
    return result;
}

long URL_HeaderParseParameters(s_URLDocument *document, char *headerValue)
{
    char *semicolonPos;
    char *equalPos;
    char *valueStart;
    char *valueEnd;
    char ch;
    int compareResult;
    int result = 0;
    
    while (1) {
        /* Find next parameter */
        semicolonPos = (char*)strchr(headerValue, ';');
        if (semicolonPos == NULL || result != 0) {
            break;
        }
        
        /* Skip whitespace after semicolon */
        do {
            semicolonPos++;
            ch = *semicolonPos;
        } while (ch == ' ' || (ch >= '\t' && ch <= '\r'));
        
        /* Find '=' for parameter value */
        equalPos = (char*)strchr(semicolonPos, '=');
        if (equalPos == NULL) {
            break;
        }
        
        if (equalPos == semicolonPos) {
            return 0;  /* Empty parameter name */
        }
        
        /* Find value start (skip quotes if present) */
        valueStart = equalPos + 1;
        if (*valueStart == '"') {
            valueStart++;
        }
        
        /* Find value end */
        valueEnd = valueStart;
        ch = *valueEnd;
        while (ch != '\0' && ch != ' ' && !(ch >= '\t' && ch <= '\r') && 
               ch != ';' && ch != '"') {
            valueEnd++;
            ch = *valueEnd;
        }
        
        /* Check if parameter is "charset" */
        compareResult = _strnicmp("charset", semicolonPos, (size_t)(equalPos - semicolonPos));
        if (compareResult == 0) {
            /* Parse charset */
            LngFindIanaFromMime((t_LngIanaCharset *)(valueEnd - valueStart), 
                               valueStart, 0);
        }
        else {
            /* Generic parameter handling */
            result = url_AddGenericHeader((s_SingleList *)(equalPos - semicolonPos), 
                                         valueStart, 
                                         (int)(valueEnd - valueStart),
                                         (char *)document, 0);
        }
        
        headerValue = valueEnd;
    }
    
    return result;
}

long url_AddGenericHeader(s_SingleList *headerList, char *headerName, unsigned int nameLength, 
                         char *headerValue, unsigned int valueLength)
{
    void *allocatedBuffer;
    unsigned int totalSize;
    char *bufferPtr;
    
    /* Calculate total size needed */
    totalSize = nameLength + valueLength + 0xF;  /* +15 for overhead */
    
    if (totalSize < nameLength + 2 ||  /* Check for overflow */
        valueLength > totalSize - nameLength - 2) {
        return -0x7FF6FFFF;
    }
    
    /* Allocate buffer */
    allocatedBuffer = UBHeap_Alloc(totalSize, 3, 0);  /* eHeapUsageString = 3 */
    if (allocatedBuffer == NULL) {
        return -0x7FF6FFFF;
    }
    
    bufferPtr = (char *)allocatedBuffer;
    
    /* Copy header name */
    memcpy(bufferPtr + 0xC, headerName, nameLength);
    bufferPtr[0xC + nameLength] = '\0';
    
    /* Store pointer to value */
    *(char **)(bufferPtr + 8) = bufferPtr + 0xC + nameLength + 1;
    
    /* Copy header value if provided */
    if (headerValue != NULL && valueLength > 0) {
        memcpy(*(char **)(bufferPtr + 8), headerValue, valueLength);
        *(*(char **)(bufferPtr + 8) + valueLength) = '\0';
    }
    else {
        **(char **)(bufferPtr + 8) = '\0';
    }
    
    /* Add to linked list */
    if (headerList->iHead == NULL) {
        headerList->iHead = (s_SingleLink *)allocatedBuffer;
    }
    else {
        *(void **)headerList->iTail = allocatedBuffer;
    }
    
    /* Update tail and clear next pointer */
    *(void **)allocatedBuffer = NULL;
    headerList->iTail = (s_SingleLink *)allocatedBuffer;
    
    return 0;
}

