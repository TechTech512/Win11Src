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

typedef enum CacheType {
    kCacheUrlDocument = 0,
    kCacheNamedAtom = 1
} CacheType;

typedef union CacheData {
    void *iDummy;
    struct s_URLDocument *iDocument;
    void *iNamedAtom;
} CacheData;

typedef struct s_CacheEntry {
    struct s_CacheEntry *iNext;
    long iLastAccess;
    unsigned int iRef;
    CacheType iType;
    CacheData iData;
} s_CacheEntry;

typedef struct s_URLDocument {
    s_ParsedURL *base;
    s_ParsedURL *request;
    int iOriginTrustLevel;
    int iTrustLevel;
    /* SecurityValues iSecurity; (placeholder) */
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
    /* s_SingleList headers; (placeholder) */
    void *iAuthenticationChallenge;
    void *iProxyAuthChallenge;
    void *scheme_ws;
    /* s_SingleList scheme_fifo; (placeholder) */
    int scheme_module_flag;
    void *iSchemePlugin;
    unsigned int state;
    void *socket;
    void *type_ws;
    /* s_SingleList type_fifo; (placeholder) */
    int type_decoder_flag;
    void *iContentPlugin;
    unsigned int type_state;
    int unget_buf;
    /* s_SingleList body; (placeholder) */
} s_URLDocument;

/* External functions */
extern void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern void UBHeap_Free(void *ptr, unsigned int param_2);
extern void mmeCache_FreeDocument(s_URLDocument *document);

/* Global variables */
s_CacheEntry *gCache_CachedDocuments = NULL;

/* Enum definitions */
typedef enum eUrlTrustLevel {
    eUrlTrustNone = 0,
    eUrlTrustLocal = 1
} eUrlTrustLevel;

/* Forward declarations for internal functions */
void lFreeEntry(s_CacheEntry *entry, s_CacheEntry *prev);
int lFindLru(s_CacheEntry **foundEntry, s_CacheEntry **prevEntry);
int lCompareUrl(s_ParsedURL *url1, s_ParsedURL *url2);
int lCheckExpired(s_CacheEntry **entryPtr, s_CacheEntry *entry, long currentTime);

int lCheckExpired(s_CacheEntry **entryPtr, s_CacheEntry *entry, long currentTime)
{
    s_URLDocument *document;
    
    if (entry == NULL) {
        return 0;
    }
    
    if (entry->iType == kCacheUrlDocument) {
        document = entry->iData.iDocument;
        
        /* Check if document has expired */
        if ((document->expires == 0) || (currentTime < document->expires) || (entry->iRef != 0)) {
            return 0;
        }
    }
    else if (entry->iRef != 0) {
        return 0;
    }
    
    /* Remove entry from list */
    if (entryPtr != NULL) {
        *entryPtr = entry->iNext;
    }
    
    lFreeEntry(entry, NULL);
    return 1;
}

int lCompareUrl(s_ParsedURL *url1, s_ParsedURL *url2)
{
    unsigned char *str1;
    unsigned char *str2;
    int result;
    
    /* Compare scheme */
    str1 = (unsigned char *)url1->scheme;
    str2 = (unsigned char *)url2->scheme;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare path */
    str1 = (unsigned char *)url1->path;
    str2 = (unsigned char *)url2->path;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare host */
    str1 = (unsigned char *)url1->host;
    str2 = (unsigned char *)url2->host;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare port */
    if (url1->port != url2->port) {
        return 0;
    }
    
    /* Compare user */
    str1 = (unsigned char *)url1->user;
    str2 = (unsigned char *)url2->user;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare password */
    str1 = (unsigned char *)url1->password;
    str2 = (unsigned char *)url2->password;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare params */
    str1 = (unsigned char *)url1->params;
    str2 = (unsigned char *)url2->params;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare query */
    str1 = (unsigned char *)url1->query;
    str2 = (unsigned char *)url2->query;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    /* Compare fragment */
    str1 = (unsigned char *)url1->fragment;
    str2 = (unsigned char *)url2->fragment;
    
    while (1) {
        if (*str1 != *str2) {
            result = (*str1 < *str2) ? -1 : 1;
            break;
        }
        if (*str1 == 0) {
            result = 0;
            break;
        }
        str1++;
        str2++;
    }
    
    if (result != 0) {
        return 0;
    }
    
    return 1;
}

int lFindLru(s_CacheEntry **foundEntry, s_CacheEntry **prevEntry)
{
    s_CacheEntry *current;
    s_CacheEntry *prev;
    s_CacheEntry *expiredEntry;
    s_CacheEntry *expiredPrev;
    s_CacheEntry *lruEntry;
    s_CacheEntry *lruPrev;
    long currentTime;
    long minAccessTime;
    long expiresTime;
    
    current = gCache_CachedDocuments;
    prev = NULL;
    expiredEntry = NULL;
    expiredPrev = NULL;
    lruEntry = NULL;
    lruPrev = NULL;
    minAccessTime = 0x7FFFFFFF;
    
    time(&currentTime);
    
    while (current != NULL) {
        if (current->iRef == 0) {
            if (current->iType == kCacheUrlDocument) {
                expiresTime = current->iData.iDocument->expires;
                
                if ((expiresTime < 1) || (currentTime <= expiresTime)) {
                    if (current->iLastAccess < minAccessTime) {
                        minAccessTime = current->iLastAccess;
                        lruEntry = current;
                        lruPrev = prev;
                    }
                }
                else {
                    /* Document has expired */
                    expiredEntry = current;
                    expiredPrev = prev;
                }
            }
            else if (current->iType == kCacheNamedAtom) {
                if (current->iLastAccess < minAccessTime) {
                    minAccessTime = current->iLastAccess;
                    lruEntry = current;
                    lruPrev = prev;
                }
            }
        }
        
        prev = current;
        current = current->iNext;
    }
    
    if (expiredEntry != NULL) {
        *foundEntry = expiredEntry;
        *prevEntry = expiredPrev;
        return 1;
    }
    
    *foundEntry = lruEntry;
    *prevEntry = lruPrev;
    return 0;
}

void lFreeEntry(s_CacheEntry *entry, s_CacheEntry *prev)
{
    s_URLDocument *document;
    
    if (entry == NULL) {
        return;
    }
    
    /* Remove from linked list */
    if (prev != NULL) {
        prev->iNext = entry->iNext;
    }
    else {
        gCache_CachedDocuments = entry->iNext;
    }
    
    if (entry->iType == kCacheUrlDocument) {
        document = entry->iData.iDocument;
        
        if (document->iTrustLevel == eUrlTrustNone) {
            mmeCache_FreeDocument(document);
        }
        else if (document->iTrustLevel == eUrlTrustLocal) {
            UBHeap_Free(document, 0);
        }
        else {
            UBHeap_Free(document, 0);
        }
    }
    
    UBHeap_Free(entry, 0);
}

long MmeCache_AddDocument(s_URLDocument *document)
{
    s_CacheEntry *entry;
    long result;
    
    entry = (s_CacheEntry *)UBHeap_Alloc(sizeof(s_CacheEntry), 9, 0);  /* eHeapUsageCacheList = 9 */
    if (entry == NULL) {
        return -0x7FFFFFFF;
    }
    
    time(&entry->iLastAccess);
    entry->iType = kCacheUrlDocument;
    entry->iData.iDocument = document;
    entry->iRef = 1;
    entry->iNext = gCache_CachedDocuments;
    
    gCache_CachedDocuments = entry;
    return 0;
}

void MmeCache_DeleteDocument(s_URLDocument *document)
{
    s_CacheEntry *current;
    s_CacheEntry *prev;
    
    current = gCache_CachedDocuments;
    prev = NULL;
    
    while (current != NULL) {
        if ((current->iType == kCacheUrlDocument) && (current->iData.iDocument == document)) {
            lFreeEntry(current, prev);
            return;
        }
        
        prev = current;
        current = current->iNext;
    }
}

int MmeCache_DiscardLru(void)
{
    s_CacheEntry *entry;
    s_CacheEntry *prev;
    int found;
    
    found = lFindLru(&entry, &prev);
    if (entry != NULL) {
        lFreeEntry(entry, prev);
    }
    
    return (entry != NULL) ? 1 : 0;
}

s_URLDocument *MmeCache_FindDocument(s_ParsedURL *url)
{
    s_CacheEntry *current;
    s_CacheEntry *prev;
    s_URLDocument *document;
    long currentTime;
    int expired;
    int match;
    
    current = gCache_CachedDocuments;
    prev = NULL;
    
    time(&currentTime);
    
    while (current != NULL) {
        if (current->iType == kCacheUrlDocument) {
            document = current->iData.iDocument;
            
            /* Check if entry has expired */
            expired = lCheckExpired(&current->iNext, current, currentTime);
            if (expired != 0) {
                /* Entry was removed, update current pointer */
                if (prev == NULL) {
                    current = gCache_CachedDocuments;
                }
                else {
                    current = prev->iNext;
                }
                continue;
            }
            
            /* Check if document matches the requested URL */
            if ((document->scheme_module_flag != 0) && ((document->flags & 0x100000) != 0)) {
                match = lCompareUrl(url, document->base);
                if (match != 0) {
                    current->iLastAccess = currentTime;
                    return document;
                }
            }
        }
        
        prev = current;
        current = current->iNext;
    }
    
    return NULL;
}

