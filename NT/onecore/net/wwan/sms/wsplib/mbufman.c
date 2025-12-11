#include <stdlib.h>
#include <string.h>

/* Structure definition */
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

/* External functions */
extern void *UBHeap_Alloc(unsigned int size, int usage, unsigned int param_3);
extern void UBHeap_Free(void *ptr, unsigned int param_2);

/* Forward declarations */
void MBufC_Free(MBuf *mbuf);

MBuf *MBufC_Alloc(int size)
{
    int numBlocks;
    MBuf *mbuf;
    MBuf *prevMbuf;
    MBuf *allocatedMbuf;
    MBuf *lastAllocated;
    
    if (size <= 0) {
        return NULL;
    }
    
    numBlocks = (size + 49) / 50;  /* Round up to nearest 50-byte block */
    prevMbuf = NULL;
    lastAllocated = NULL;
    
    while (numBlocks > 0) {
        mbuf = (MBuf *)UBHeap_Alloc(sizeof(MBuf), 7, 0);  /* eHeapUsageMBuf = 7 */
        if (mbuf == NULL) {
            /* Allocation failed, free any already allocated blocks */
            MBufC_Free(lastAllocated);
            return NULL;
        }
        
        mbuf->length = 0;
        mbuf->start = 0;
        mbuf->iOwners = 1;
        mbuf->next = prevMbuf;
        
        if (numBlocks < 2) {
            /* Last block may be partial */
            int remaining = size % 50;
            if (remaining == 0) {
                remaining = 50;
            }
            mbuf->length = remaining;
        }
        else {
            mbuf->length = 50;
        }
        
        numBlocks--;
        prevMbuf = mbuf;
        lastAllocated = mbuf;
    }
    
    return prevMbuf;
}

int MBufC_CopyIn(MBuf *mbufChain, unsigned char *data, int offset, int size)
{
    int totalLength;
    MBuf *currentMbuf;
    MBuf *lastMbuf;
    MBuf *newMbuf;
    int currentOffset;
    int copyOffset;
    int copySize;
    unsigned char *destPtr;
    unsigned char *srcPtr;
    int bytesCopied;
    
    if ((mbufChain == NULL) || (data == NULL) || (size < 1)) {
        return -0x7FE8FFFE;  /* Error code */
    }
    
    /* Calculate total length of the mbuf chain */
    totalLength = 0;
    currentMbuf = mbufChain;
    while (currentMbuf != NULL) {
        totalLength += currentMbuf->length;
        currentMbuf = currentMbuf->next;
    }
    
    /* If chain is too short, allocate more mbufs */
    bytesCopied = 0;
    if (totalLength < size) {
        newMbuf = MBufC_Alloc(size - totalLength);
        if (newMbuf == NULL) {
            return -0x7FE8FFFF;  /* Error code */
        }
        
        /* Find end of chain and append new mbufs */
        currentMbuf = mbufChain;
        while (currentMbuf->next != NULL) {
            currentMbuf = currentMbuf->next;
        }
        currentMbuf->next = newMbuf;
    }
    
    /* Find starting mbuf based on offset */
    currentOffset = 0;
    currentMbuf = mbufChain;
    while (currentMbuf != NULL) {
        if (offset < currentOffset + currentMbuf->length) {
            break;
        }
        currentOffset += currentMbuf->length;
        currentMbuf = currentMbuf->next;
    }
    
    /* Copy data into mbuf chain */
    while ((currentMbuf != NULL) && (bytesCopied < size)) {
        if (offset < totalLength) {
            copyOffset = offset;
            if (offset < 0) {
                copyOffset = 0;
            }
            copyOffset -= offset;
            
            srcPtr = data + bytesCopied;
            destPtr = currentMbuf->buf + currentMbuf->start + copyOffset;
            copySize = currentMbuf->length - copyOffset;
            
            if (bytesCopied + copySize > size) {
                copySize = size - bytesCopied;
            }
        }
        else {
            copyOffset = 0;
            srcPtr = data + bytesCopied;
            destPtr = currentMbuf->buf + currentMbuf->start;
            copySize = currentMbuf->length;
            
            if (bytesCopied + copySize > size) {
                copySize = size - bytesCopied;
            }
            currentMbuf->length = copySize;
        }
        
        /* Validate copy parameters */
        if ((currentMbuf->start + copyOffset < currentMbuf->start) ||
            (copyOffset + offset < copyOffset) ||
            (copyOffset + offset < 0) ||
            (copySize < copyOffset) ||
            (currentMbuf->start + copySize > 50)) {
            return -0x7FE8FFFF;  /* Error code */
        }
        
        memcpy(destPtr, srcPtr, copySize);
        bytesCopied += copySize;
        offset += copySize;
        currentMbuf = currentMbuf->next;
    }
    
    return 0;
}

int MBufC_CopyOut(MBuf *mbufChain, unsigned char *buffer, int offset, int size)
{
    unsigned char *srcPtr;
    MBuf *currentMbuf;
    unsigned char *destPtr;
    MBuf *targetMbuf;
    unsigned char *copyEnd;
    int bytesCopied;
    unsigned char *copyStart;
    
    if ((mbufChain == NULL) || (buffer == NULL) || (size < 1)) {
        return -0x7FE8FFFE;  /* Error code */
    }
    
    /* Find starting mbuf based on offset */
    currentMbuf = mbufChain;
    targetMbuf = NULL;
    while (currentMbuf != NULL) {
        if (offset < (int)(currentMbuf->buf - (unsigned char *)currentMbuf + currentMbuf->length - 16)) {
            break;
        }
        currentMbuf = currentMbuf->next;
    }
    
    bytesCopied = 0;
    destPtr = buffer;
    
    if (currentMbuf != NULL) {
        srcPtr = (unsigned char *)mbufChain + offset;
        
        while (currentMbuf != NULL) {
            if (destPtr >= buffer + size) {
                break;
            }
            
            copyStart = currentMbuf->buf;
            if (srcPtr > copyStart) {
                copyStart = srcPtr;
            }
            
            copyEnd = currentMbuf->buf + currentMbuf->length - 16;
            if (destPtr + (copyEnd - copyStart) > buffer + size) {
                copyEnd = copyStart + (buffer + size - destPtr);
            }
            
            if (copyEnd > copyStart) {
                memcpy(destPtr, copyStart, copyEnd - copyStart);
                destPtr += (copyEnd - copyStart);
                bytesCopied += (copyEnd - copyStart);
            }
            
            srcPtr = currentMbuf->buf + currentMbuf->length - 16;
            currentMbuf = currentMbuf->next;
        }
    }
    
    return bytesCopied;
}

void MBufC_Free(MBuf *mbufChain)
{
    MBuf *currentMbuf;
    MBuf *nextMbuf;
    
    currentMbuf = mbufChain;
    
    while (currentMbuf != NULL) {
        currentMbuf->iOwners--;
        
        if (currentMbuf->iOwners == 0) {
            nextMbuf = currentMbuf->next;
            UBHeap_Free(currentMbuf, 7);  /* eHeapUsageMBuf = 7 */
            currentMbuf = nextMbuf;
        }
        else {
            currentMbuf = currentMbuf->next;
        }
    }
}

