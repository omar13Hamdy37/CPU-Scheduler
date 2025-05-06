#ifndef PRIQUEUE_H
#define PRIQUEUE_H

#include <stdlib.h>
#include <stdio.h>

typedef struct PriNode {
    void* item;
    int pri;
    struct PriNode* next;
} PriNode;

typedef struct {
    PriNode* head;
} PriQueue;


PriQueue* createPriQueue();


void destroyPriQueue(PriQueue* q);


void enqueuePri(PriQueue* q, void* item, int pri);


int dequeuePri(PriQueue* q, void** item, int* pri);


int peekPri(PriQueue* q, void** item, int* pri);


int isEmptyPri(PriQueue* q);


void printPointersPri(PriQueue* q, void (*printFn)( void*)) ;


int countPri(PriQueue* q);

#endif
