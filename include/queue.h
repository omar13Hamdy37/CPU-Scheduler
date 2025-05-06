#include <stdbool.h>

typedef struct QueueNode {
    void* data;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
} Queue;

Queue* createQueue();
void enqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);
void* peek(Queue* queue);
bool isEmpty(Queue* queue);
int getQueueSize(Queue* queue);
void destroyQueue(Queue* queue);
void removeNode(Queue* q, QueueNode* minNode);



