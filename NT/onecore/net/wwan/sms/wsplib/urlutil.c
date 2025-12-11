#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/* External functions */
extern void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern void UBHeap_Free(void *ptr, unsigned int param_2);
extern void *lRealloc(void *ptr, unsigned int size, int param_3, unsigned int param_4, int param_5);

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

void lInitParsedUrl(s_ParsedURL *url)
{
    if (url == NULL) {
        return;
    }
    
    /* Initialize all pointers to point to the iUrlComponents buffer */
    url->user = url->iUrlComponents;
    url->password = url->iUrlComponents;
    url->scheme = url->iUrlComponents;
    url->host = url->iUrlComponents;
    url->path = url->iUrlComponents;
    url->params = url->iUrlComponents;
    url->query = url->iUrlComponents;
    url->fragment = url->iUrlComponents;
    
    url->port = 80;  /* Default HTTP port */
    url->iUrlLength = 0;
    url->iUrlHasScheme = 0;
    url->iUrlHasNetLoc = 0;
    url->iUrlHasPort = 0;
    url->iUrlComponents[0] = '\0';
}

long lUrl_Parse(s_ParsedURL **result, char *urlString, int param_3, int param_4)
{
    char *current;
    char *end;
    char *fragmentPos;
    char *queryPos;
    char *colonPos;
    char *slashPos;
    char *atPos;
    char *portPos;
    char *semicolonPos;
    char *temp;
    s_ParsedURL *parsedUrl;
    char *bufferPtr;
    unsigned int urlLength;
    long status;
    int hasScheme;
    size_t copyLen;
    
    if (result == NULL || urlString == NULL) {
        return -0x7FF2FFFF;
    }
    
    *result = NULL;
    
    /* Skip leading whitespace */
    current = urlString;
    while (*current == ' ' || (*current >= '\t' && *current <= '\r')) {
        current++;
    }
    
    /* Find end of string and trim trailing whitespace */
    end = current;
    while (*end != '\0') {
        end++;
    }
    
    while (end > current && (*end == ' ' || (*end >= '\t' && *end <= '\r'))) {
        end--;
    }
    end++;
    
    urlLength = (unsigned int)(end - current);
    
    /* Allocate memory for parsed URL */
    parsedUrl = (s_ParsedURL *)UBHeap_Alloc(sizeof(s_ParsedURL) + urlLength, eHeapUsageURL, 0);
    if (parsedUrl == NULL) {
        return -0x7FF2FFFF;
    }
    
    *result = parsedUrl;
    lInitParsedUrl(parsedUrl);
    
    parsedUrl->iUrlLength = urlLength;
    bufferPtr = parsedUrl->iUrlComponents;
    
    /* Find fragment (#) */
    fragmentPos = strchr(current, '#');
    if (fragmentPos != NULL && fragmentPos < end) {
        copyLen = (size_t)(end - fragmentPos - 1);
        if (copyLen > 0) {
            memcpy(bufferPtr, fragmentPos + 1, copyLen);
        }
        bufferPtr[copyLen] = '\0';
        parsedUrl->fragment = bufferPtr;
        bufferPtr += copyLen + 1;
        end = fragmentPos;
    }
    
    /* Find query (?) */
    queryPos = strchr(current, '?');
    if (queryPos != NULL && queryPos < end) {
        copyLen = (size_t)(end - queryPos - 1);
        if (copyLen > 0) {
            memcpy(bufferPtr, queryPos + 1, copyLen);
        }
        bufferPtr[copyLen] = '\0';
        parsedUrl->query = bufferPtr;
        bufferPtr += copyLen + 1;
        end = queryPos;
    }
    
    /* Check for scheme (protocol) */
    colonPos = strchr(current, ':');
    if (colonPos != NULL && colonPos < end) {
        /* Validate scheme characters */
        temp = current;
        while (temp < colonPos) {
            char ch = *temp;
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || 
                  ch == '+' || ch == '-' || ch == '.')) {
                break;
            }
            temp++;
        }
        
        if (temp == colonPos) {
            parsedUrl->iUrlHasScheme = 1;
            copyLen = (size_t)(colonPos - current);
            if (copyLen > 0) {
                memcpy(bufferPtr, current, copyLen);
            }
            bufferPtr[copyLen] = '\0';
            parsedUrl->scheme = bufferPtr;
            bufferPtr += copyLen + 1;
            current = colonPos + 1;
        }
    }
    
    /* If no scheme was found and we need to provide a default */
    if (parsedUrl->scheme[0] == '\0' && result != NULL) {
        parsedUrl->scheme = bufferPtr;
        status = StringCchCopyA(bufferPtr, urlLength, "http");
        if (status < 0) {
            UBHeap_Free(parsedUrl, 0);
            return -0x7FF2FFFE;
        }
        bufferPtr += 5;  /* "http" + null terminator */
    }
    
    /* Check for "//" indicating network location */
    if (strncmp(current, "//", 2) == 0) {
        current += 2;  /* Skip "//" */
        parsedUrl->iUrlHasNetLoc = 1;
    }
    
    /* Find path separator */
    slashPos = strchr(current, '/');
    if (slashPos == NULL || slashPos >= end) {
        semicolonPos = strchr(current, ';');
        if (semicolonPos != NULL && semicolonPos < end) {
            slashPos = semicolonPos;
        }
        else {
            slashPos = end;
        }
    }
    
    /* Check for userinfo (user:password@) */
    atPos = strchr(current, '@');
    if (atPos != NULL && atPos < slashPos) {
        /* Check for password separator */
        portPos = strchr(current, ':');
        if (portPos != NULL && portPos < atPos) {
            /* Extract user */
            size_t userLen = (size_t)(atPos - portPos - 1);
            if (userLen > 0) {
                memcpy(bufferPtr, portPos + 1, userLen);
            }
            bufferPtr[userLen] = '\0';
            parsedUrl->password = bufferPtr;
            bufferPtr += userLen + 1;
            
            /* Extract password */
            size_t passLen = (size_t)(portPos - current);
            if (passLen > 0) {
                memcpy(bufferPtr, current, passLen);
            }
            bufferPtr[passLen] = '\0';
            parsedUrl->user = bufferPtr;
            bufferPtr += passLen + 1;
            
            current = atPos + 1;
        }
        else {
            /* No password, just user */
            size_t userLen = (size_t)(atPos - current);
            if (userLen > 0) {
                memcpy(bufferPtr, current, userLen);
            }
            bufferPtr[userLen] = '\0';
            parsedUrl->user = bufferPtr;
            bufferPtr += userLen + 1;
            current = atPos + 1;
        }
    }
    
    /* Check for port */
    portPos = strchr(current, ':');
    if (portPos != NULL && portPos < slashPos) {
        parsedUrl->iUrlHasPort = 1;
        parsedUrl->port = 0;
        
        temp = portPos + 1;
        while (temp < slashPos && *temp >= '0' && *temp <= '9') {
            parsedUrl->port = parsedUrl->port * 10 + (*temp - '0');
            temp++;
        }
    }
    
    /* Extract host */
    char *hostEnd = (portPos != NULL && portPos < slashPos) ? portPos : slashPos;
    copyLen = (size_t)(hostEnd - current);
    if (copyLen > 0) {
        memcpy(bufferPtr, current, copyLen);
    }
    bufferPtr[copyLen] = '\0';
    parsedUrl->host = bufferPtr;
    bufferPtr += copyLen + 1;
    
    /* Check for parameters (;) */
    semicolonPos = strchr(current, ';');
    if (semicolonPos != NULL && semicolonPos < end) {
        copyLen = (size_t)(end - semicolonPos - 1);
        if (copyLen > 0) {
            memcpy(bufferPtr, semicolonPos + 1, copyLen);
        }
        bufferPtr[copyLen] = '\0';
        parsedUrl->params = bufferPtr;
        bufferPtr += copyLen + 1;
        end = semicolonPos;
    }
    
    /* Extract path */
    if (slashPos == current) {
        if (param_3 == 0 || parsedUrl->iUrlHasScheme != 0) {
            parsedUrl->path = bufferPtr;
            status = StringCchCopyA(bufferPtr, urlLength, "/");
            if (status < 0) {
                UBHeap_Free(parsedUrl, 0);
                return -0x7FF2FFFE;
            }
        }
        else {
            *bufferPtr = '\0';
            parsedUrl->path = bufferPtr;
        }
    }
    else {
        parsedUrl->path = bufferPtr;
        copyLen = (size_t)(end - slashPos);
        if (copyLen > 0) {
            memcpy(bufferPtr, slashPos, copyLen);
            bufferPtr[copyLen] = '\0';
        }
        else {
            *bufferPtr = '\0';
        }
    }
    
    return 0;
}

long url_parse_partial(s_ParsedURL **result, s_ParsedURL *baseUrl, char *relativeUrl)
{
    s_ParsedURL *parsedRelative = NULL;
    s_ParsedURL *combinedUrl = NULL;
    char *bufferPtr;
    char *temp;
    unsigned int totalLength;
    int componentCount;
    long status;
    int i;
    
    if (result == NULL || baseUrl == NULL || relativeUrl == NULL) {
        return -0x7FF2FFFF;
    }
    
    *result = NULL;
    
    /* Parse the relative URL */
    status = lUrl_Parse(&parsedRelative, relativeUrl, 0, 0);
    if (status != 0) {
        return status;
    }
    
    /* Check if the parsed relative URL has a scheme */
    if (parsedRelative->iUrlHasScheme != 0) {
        /* Relative URL has its own scheme, use it as-is */
        *result = parsedRelative;
        return 0;
    }
    
    /* Calculate total length needed for combined URL */
    totalLength = baseUrl->iUrlLength + parsedRelative->iUrlLength + 100;
    
    /* Allocate memory for combined URL */
    combinedUrl = (s_ParsedURL *)UBHeap_Alloc(sizeof(s_ParsedURL) + totalLength, eHeapUsageURL, 0);
    if (combinedUrl == NULL) {
        UBHeap_Free(parsedRelative, 0);
        return -0x7FF2FFFF;
    }
    
    lInitParsedUrl(combinedUrl);
    bufferPtr = combinedUrl->iUrlComponents;
    componentCount = 0;
    
    /* Copy scheme from base URL */
    combinedUrl->scheme = bufferPtr;
    status = StringCchCopyA(bufferPtr, totalLength, baseUrl->scheme);
    if (status < 0) {
        UBHeap_Free(parsedRelative, 0);
        UBHeap_Free(combinedUrl, 0);
        return -0x7FF2FFFF;
    }
    
    temp = bufferPtr;
    while (*temp != '\0') {
        temp++;
    }
    bufferPtr = temp + 1;
    componentCount++;
    
    combinedUrl->port = baseUrl->port;
    
    /* Copy user from base URL if present */
    if (baseUrl->user[0] != '\0' && baseUrl->user != baseUrl->iUrlComponents) {
        combinedUrl->user = bufferPtr;
        status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                               baseUrl->user);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        temp = bufferPtr;
        while (*temp != '\0') {
            temp++;
        }
        bufferPtr = temp + 1;
        componentCount++;
        combinedUrl->iUrlHasNetLoc = 1;
    }
    
    /* Copy password from base URL if present */
    if (baseUrl->password[0] != '\0' && baseUrl->password != baseUrl->iUrlComponents) {
        combinedUrl->password = bufferPtr;
        status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                               baseUrl->password);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        temp = bufferPtr;
        while (*temp != '\0') {
            temp++;
        }
        bufferPtr = temp + 1;
        componentCount++;
        combinedUrl->iUrlHasNetLoc = 1;
    }
    
    /* Copy host from base URL */
    if (baseUrl->host[0] != '\0' && baseUrl->host != baseUrl->iUrlComponents) {
        combinedUrl->host = bufferPtr;
        status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                               baseUrl->host);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        temp = bufferPtr;
        while (*temp != '\0') {
            temp++;
        }
        bufferPtr = temp + 1;
        componentCount++;
        combinedUrl->iUrlHasNetLoc = 1;
    }
    
    /* Handle path - check if relative URL has an absolute path */
    if (parsedRelative->path[0] == '/') {
        /* Absolute path in relative URL */
        combinedUrl->path = bufferPtr;
        status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                               parsedRelative->path);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        temp = bufferPtr;
        while (*temp != '\0') {
            temp++;
        }
        
        /* Calculate total URL length */
        combinedUrl->iUrlLength = (unsigned int)(temp + 1 - combinedUrl->iUrlComponents - componentCount - 0x34);
    }
    else if (parsedRelative->path[0] != '\0') {
        /* Relative path - need to combine with base path */
        char *combinedPath = bufferPtr;
        
        /* Start with base URL path */
        status = StringCchCopyA(combinedPath, totalLength - (unsigned int)(combinedPath - combinedUrl->iUrlComponents), 
                               baseUrl->path);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        /* Find last slash in base path */
        char *lastSlash = strrchr(combinedPath, '/');
        if (lastSlash != NULL) {
            /* Truncate at last slash */
            lastSlash[1] = '\0';
        }
        else {
            /* No slash in path, append one */
            size_t len = strlen(combinedPath);
            if (len + 1 < totalLength - (unsigned int)(combinedPath - combinedUrl->iUrlComponents)) {
                combinedPath[len] = '/';
                combinedPath[len + 1] = '\0';
            }
        }
        
        /* Append relative path */
        status = StringCchCopyA(combinedPath + strlen(combinedPath), 
                               totalLength - (unsigned int)(combinedPath + strlen(combinedPath) - combinedUrl->iUrlComponents),
                               parsedRelative->path);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        /* Remove "./" segments */
        char *dotSlash = strstr(combinedPath, "./");
        while (dotSlash != NULL) {
            if (dotSlash == combinedPath || dotSlash[-1] == '/') {
                /* Remove "./" */
                memmove(dotSlash, dotSlash + 2, strlen(dotSlash + 2) + 1);
                dotSlash = strstr(dotSlash, "./");
            }
            else {
                dotSlash += 2;
                dotSlash = strstr(dotSlash, "./");
            }
        }
        
        /* Remove trailing "/." or handle "." */
        int pathLen = (int)strlen(combinedPath);
        if ((pathLen > 1 && combinedPath[pathLen - 1] == '.' && combinedPath[pathLen - 2] == '/') ||
            (pathLen == 1 && combinedPath[0] == '.')) {
            combinedPath[pathLen - 1] = '\0';
            pathLen--;
        }
        
        /* Remove "/../" segments */
        char *dotDotSlash = strstr(combinedPath, "/../");
        while (dotDotSlash != NULL) {
            int segmentStart = (int)(dotDotSlash - combinedPath);
            if (segmentStart >= 3 && combinedPath[segmentStart - 1] == '.' && 
                combinedPath[segmentStart - 2] == '.' && combinedPath[segmentStart - 3] == '/') {
                /* Already a "../", skip */
                dotDotSlash += 4;
            }
            else {
                /* Find previous slash */
                int prevSlash = segmentStart;
                while (prevSlash > 0 && combinedPath[prevSlash] != '/') {
                    prevSlash--;
                }
                
                if (combinedPath[prevSlash] == '/') {
                    /* Remove segment */
                    memmove(combinedPath + prevSlash + 1, dotDotSlash + 4, strlen(dotDotSlash + 4) + 1);
                }
                else {
                    /* At beginning, just remove leading path */
                    combinedPath[0] = '\0';
                }
            }
            dotDotSlash = strstr(combinedPath, "/../");
        }
        
        /* Check for "/.." at end */
        pathLen = (int)strlen(combinedPath);
        if (pathLen >= 3 && strcmp(combinedPath + pathLen - 3, "/..") == 0) {
            if (pathLen == 3) {
                combinedPath[0] = '\0';
            }
            else {
                /* Find previous slash */
                int prevSlash = pathLen - 4;
                while (prevSlash > 0 && combinedPath[prevSlash] != '/') {
                    prevSlash--;
                }
                
                if (prevSlash >= 0) {
                    combinedPath[prevSlash + 1] = '\0';
                }
            }
        }
        
        combinedUrl->path = combinedPath;
        
        /* Calculate total URL length */
        temp = combinedPath;
        while (*temp != '\0') {
            temp++;
        }
        
        combinedUrl->iUrlLength = (unsigned int)(temp + 1 - combinedUrl->iUrlComponents - componentCount - 0x34);
    }
    else {
        /* Empty path in relative URL - use base URL path */
        combinedUrl->path = bufferPtr;
        status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                               baseUrl->path);
        if (status < 0) {
            UBHeap_Free(parsedRelative, 0);
            UBHeap_Free(combinedUrl, 0);
            return -0x7FF2FFFF;
        }
        
        temp = bufferPtr;
        while (*temp != '\0') {
            temp++;
        }
        bufferPtr = temp + 1;
        componentCount++;
        
        /* Copy params from relative URL if present, otherwise from base URL */
        if (parsedRelative->params[0] != '\0' && parsedRelative->params != parsedRelative->iUrlComponents) {
            combinedUrl->params = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   parsedRelative->params);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        else if (baseUrl->params[0] != '\0' && baseUrl->params != baseUrl->iUrlComponents) {
            combinedUrl->params = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   baseUrl->params);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        
        /* Copy query from relative URL if present, otherwise from base URL */
        if (parsedRelative->query[0] != '\0' && parsedRelative->query != parsedRelative->iUrlComponents) {
            combinedUrl->query = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   parsedRelative->query);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        else if (baseUrl->query[0] != '\0' && baseUrl->query != baseUrl->iUrlComponents) {
            combinedUrl->query = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   baseUrl->query);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        
        /* Copy fragment from relative URL if present, otherwise from base URL */
        if (parsedRelative->fragment[0] != '\0' && parsedRelative->fragment != parsedRelative->iUrlComponents) {
            combinedUrl->fragment = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   parsedRelative->fragment);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        else if (baseUrl->fragment[0] != '\0' && baseUrl->fragment != baseUrl->iUrlComponents) {
            combinedUrl->fragment = bufferPtr;
            status = StringCchCopyA(bufferPtr, totalLength - (unsigned int)(bufferPtr - combinedUrl->iUrlComponents), 
                                   baseUrl->fragment);
            if (status < 0) {
                UBHeap_Free(parsedRelative, 0);
                UBHeap_Free(combinedUrl, 0);
                return -0x7FF2FFFF;
            }
            
            temp = bufferPtr;
            while (*temp != '\0') {
                temp++;
            }
            bufferPtr = temp + 1;
            componentCount++;
        }
        
        combinedUrl->iUrlLength = (unsigned int)(bufferPtr - combinedUrl->iUrlComponents - componentCount - 0x34);
    }
    
    /* Clean up temporary parsed relative URL */
    UBHeap_Free(parsedRelative, 0);
    
    /* Set result */
    *result = combinedUrl;
    return 0;
}

