#include <windows.h>

/* Structure definitions */
typedef struct s_SingleLink {
    struct s_SingleLink *iNext;
} s_SingleLink;

typedef struct s_SingleList {
    s_SingleLink *iHead;
    s_SingleLink *iTail;
} s_SingleList;

typedef struct s_aneframe {
    unsigned long iWidth;
    unsigned long iHeight;
    unsigned long iLeftOffset;
    unsigned long iTopOffset;
    unsigned long iDuration;
    struct s_aneframe *iNextFrame_h;
    int iFinished;
    int iFrameDisposal;
    unsigned long iSegmentCount;
    void *iSegments[1];
} s_aneframe;

typedef enum t_FrameDispose {
    eDisposeNone = 0,
    eDisposeBackground = 1,
    eDisposePrevious = 2
} t_FrameDispose;

typedef struct s_aneanimtok {
    s_SingleLink *iLink;
    unsigned long iTokid;
    void *iROMResourceData;
    unsigned long iWidth;
    unsigned long iHeight;
    unsigned long iFramecount;
    unsigned long iIterations;
    unsigned long iCurrentLoop;
    s_aneframe *iFirstFrame_h;
    int iFinished;
    unsigned char iBGColorIndex;
    void *iPreviousFrame;
    int iUseTransparency;
} s_aneanimtok;

typedef enum valueType {
    EUndefined = 0,
    EInvalid = 1,
    EInteger = 2,
    ENumber = 3,
    EString = 4,
    EString4 = 5,
    EBoolean = 6,
    EExternalString = 7
} valueType;

typedef union valueData {
    void *iDummy;
    long iInteger;
    float iNumber;
    unsigned short *iString;
    void *iString4;
    int iBoolean;
} valueData;

typedef struct _swmlsValue {
    valueType iType;
    valueData iData;
    int iCopy;
} _swmlsValue;

typedef struct _sFunctionPool {
    unsigned char iNumFunc;
    unsigned char iNumNames;
    void *iFunctionNameTable;
    void *iFunctions;
    unsigned short *iFunctionNameBlock;
    char *iFunctionBlock;
} _sFunctionPool;

typedef struct _sWMLScriptData {
    unsigned char iScrpVersion;
    unsigned short iCharacterSet;
    _swmlsValue *iConstantPool;
    unsigned short iNumConstants;
    void *iPragmaPool;
    unsigned short iNumPragmas;
    _sFunctionPool *iFunctionPool;
} _sWMLScriptData;

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

/* External functions */
extern void UBHeap_Free(void *ptr, unsigned int param_2);
extern s_SingleLink *SingleListRemoveHead(s_SingleList *list);

/* Function forward declarations */
void lDestroyHTMLToken(void *token);
void lDestroyImageToken(void *token);
void lDestroyScriptToken(void *token);
void lFreeAnimToken(s_aneanimtok *token);
void lFreeConstantPool(_sWMLScriptData *data);
void MmeCache_FreeFIFO(s_SingleList *list, unsigned int param_2);

void lDestroyHTMLToken(void *token)
{
    unsigned int tokenType;
    unsigned int counter;
    int *arrayPtr;
    void *childToken;
    
    tokenType = *(unsigned int *)((char *)token + 4);
    
    if (tokenType < 0x11) {
        if (tokenType == 0x10) {
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            
            if ((*(int *)((char *)token + 0x30) != *(int *)((char *)token + 0x3c)) &&
                (*(int *)((char *)token + 0x30) != 0)) {
                if (*(int *)((char *)token + 0x18) == 5) {
                    counter = 0;
                    arrayPtr = *(int **)((char *)token + 0x48);
                    if (*(int *)((char *)token + 0x24) != 0) {
                        do {
                            UBHeap_Free(NULL, 0);
                            if ((arrayPtr != NULL) && (*arrayPtr != 0)) {
                                UBHeap_Free(NULL, 0);
                            }
                            arrayPtr = arrayPtr + 1;
                            counter = counter + 1;
                        } while (counter < *(unsigned int *)((char *)token + 0x24));
                    }
                }
                UBHeap_Free(NULL, 0);
            }
            UBHeap_Free(NULL, 0);
            
            childToken = (void *)0x10003877;
            UBHeap_Free(NULL, 0);
            
            if (*(int *)((char *)token + 0x4c) != 0) {
                if (*(int *)((char *)token + 0x18) == 7) {
                    childToken = *(void **)((char *)token + 0x4c);
                    lDestroyHTMLToken(childToken);
                }
                UBHeap_Free(childToken, 0);
            }
            UBHeap_Free(childToken, 0);
            return;
        }
        
        if (tokenType < 0x0B) {
            if (tokenType != 0x0A) {
                if (tokenType == 0x03) {
                    UBHeap_Free(NULL, 0);
                    UBHeap_Free(NULL, 0);
                    UBHeap_Free(NULL, 0);
                    UBHeap_Free(NULL, 0);
                }
                else if (tokenType != 0x05) {
                    if (tokenType == 0x06) {
                        UBHeap_Free(NULL, 0);
                        UBHeap_Free(NULL, 0);
                    }
                    else {
                        if (tokenType != 0x07) {
                            return;
                        }
                        UBHeap_Free(NULL, 0);
                    }
                }
            }
        }
        else if (tokenType == 0x0B) {
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
        }
        else if (tokenType == 0x0C) {
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
        }
        else {
            if (tokenType != 0x0F) {
                return;
            }
            UBHeap_Free(NULL, 0);
        }
    }
    else if (tokenType < 0x1E) {
        if (tokenType != 0x1D) {
            if (tokenType == 0x11) {
                UBHeap_Free(NULL, 0);
                UBHeap_Free(NULL, 0);
                UBHeap_Free(NULL, 0);
                UBHeap_Free(NULL, 0);
                UBHeap_Free(NULL, 0);
            }
            else {
                if (tokenType == 0x1A) {
                    goto free_single;
                }
                if (tokenType != 0x1B) {
                    if (tokenType != 0x1C) {
                        return;
                    }
                    goto free_single;
                }
            }
            UBHeap_Free(NULL, 0);
            UBHeap_Free(NULL, 0);
        }
    }
    else if (((tokenType != 0x1E) && (tokenType != 0x20)) && (tokenType != 0x21)) {
        return;
    }

free_single:
    UBHeap_Free(NULL, 0);
    return;
}

void lDestroyImageToken(void *token)
{
    int tokenType;
    int frameCount;
    
    tokenType = *(int *)((char *)token + 4);
    
    if (tokenType == 0x0E) {
        for (frameCount = *(int *)((char *)token + 0x14); frameCount != 0; frameCount = frameCount - 1) {
            UBHeap_Free(NULL, 0);
        }
    }
    else if (tokenType != 0x0F) {
        if (tokenType == 0x11) {
            lFreeAnimToken(NULL);
        }
        else if (tokenType == 0x12) {
            UBHeap_Free(NULL, 0);
        }
    }
    return;
}

void lDestroyScriptToken(void *token)
{
    if (*(int *)((char *)token + 8) != 0) {
        lFreeConstantPool(NULL);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
        UBHeap_Free(NULL, 0);
    }
    return;
}

void lFreeAnimToken(s_aneanimtok *token)
{
    s_aneframe *currentFrame;
    int segmentCount;
    
    currentFrame = token->iFirstFrame_h;
    
    while (currentFrame != NULL) {
        for (segmentCount = currentFrame->iSegmentCount; segmentCount != 0; segmentCount = segmentCount - 1) {
            UBHeap_Free(NULL, 0);
        }
        currentFrame = currentFrame->iNextFrame_h;
        UBHeap_Free(NULL, 0);
    }
    
    UBHeap_Free(NULL, 0);
    return;
}

void lFreeConstantPool(_sWMLScriptData *data)
{
    _swmlsValue *constant;
    unsigned int i;
    
    constant = data->iConstantPool;
    
    if (constant != NULL) {
        for (i = 0; i < data->iNumConstants; i++) {
            if (constant->iType == 4) { /* EString */
                UBHeap_Free(NULL, 0);
            }
            constant = constant + 1;
        }
        UBHeap_Free(NULL, 0);
    }
    return;
}

void MmeCache_FreeFIFO(s_SingleList *list, unsigned int param_2)
{
    s_SingleLink *link;
    void (*destroyFunc)(void *);
    
    if (1 < param_2) {
        if (param_2 < 5) {
            destroyFunc = lDestroyHTMLToken;
        }
        else if (param_2 == 5) {
            destroyFunc = lDestroyImageToken;
        }
        else if (param_2 == 7) {
            destroyFunc = lDestroyScriptToken;
        }
        else {
            destroyFunc = NULL;
        }
    }
    else {
        destroyFunc = NULL;
    }
    
    while (1) {
        link = SingleListRemoveHead(list);
        if (link == NULL) {
            break;
        }
        
        if (destroyFunc != NULL) {
            (*destroyFunc)(link);
        }
        
        UBHeap_Free(NULL, 0);
    }
    
    return;
}

void mmeCache_FreeDocument(s_URLDocument *document)
{
    if (document->base != document->request) {
        UBHeap_Free(NULL, 0);
    }
    
    UBHeap_Free(NULL, 0);
    MmeCache_FreeFIFO(NULL, 0);
    MmeCache_FreeFIFO(NULL, 0);
    MmeCache_FreeFIFO(NULL, 0);
    MmeCache_FreeFIFO(NULL, 0);
    UBHeap_Free(NULL, 0);
    UBHeap_Free(NULL, 0);
    UBHeap_Free(NULL, 0);
    return;
}

