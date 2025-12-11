#include <stddef.h>

typedef struct s_SingleLink {
    struct s_SingleLink *iNext;
    // Add other members as needed based on your actual structure
} s_SingleLink;

typedef struct s_SingleList {
    s_SingleLink *head;
    s_SingleLink *tail;  // Inferred from the code accessing in_ECX[1]
} s_SingleList;

s_SingleLink *SingleListRemoveHead(s_SingleList *list)
{
    s_SingleLink *removedNode;
    s_SingleLink *newHead;
    
    removedNode = list->head;
    if (removedNode != NULL) {
        newHead = removedNode->iNext;
        list->head = newHead;
        if (newHead == NULL) {
            list->tail = NULL;
        }
        removedNode->iNext = NULL;
    }
    return removedNode;
}

