#include "priQueue.h"

PriQueue* createPriQueue() {
    PriQueue* q = (PriQueue*)malloc(sizeof(PriQueue));
    q->head = NULL;
    return q;
}

void destroyPriQueue(PriQueue* q) {
    void* tmp;
    int p;
    while (dequeuePri(q, &tmp, &p));
    free(q);
}

void enqueuePri(PriQueue* q, void* item, int pri) {
    PriNode* newNode = (PriNode*)malloc(sizeof(PriNode));
    newNode->item = item;
    newNode->pri = pri;
    newNode->next = NULL;

    if (!q->head || pri > q->head->pri) {
        newNode->next = q->head;
        q->head = newNode;
        return;
    }

    PriNode* current = q->head;
    while (current->next && pri <= current->next->pri) {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
}

int dequeuePri(PriQueue* q, void** item, int* pri) {
    if (!q->head)
        return 0;

    PriNode* temp = q->head;
    *item = temp->item;
    *pri = temp->pri;
    q->head = q->head->next;
    free(temp);
    return 1;
}

int peekPri(PriQueue* q, void** item, int* pri) {
    if (!q->head)
        return 0;
    *item = q->head->item;
    *pri = q->head->pri;
    return 1;
}

int isEmptyPri(PriQueue* q) {
    return q->head == NULL;
}

// ok so here i want to print the actual content of the node
// change it so that i pass a function that prints that node (ana ely ba define the func)

void printPointersPri(PriQueue* q, void (*printFn)( void*))  {
    PriNode* current = q->head;
    while (current) {
        printf("Priority %d: ", current->pri);
        printFn(current->item); 
        printf("\n");
        current = current->next;
    }
}


int countPri(PriQueue* q) {
    int cnt = 0;
    PriNode* current = q->head;
    while (current) {
        cnt++;
        current = current->next;
    }
    return cnt;
}
