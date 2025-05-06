#include "queue.h"
#include <stdlib.h>
#include <stdio.h>


Queue* createQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        perror("Failed to create queue");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

void enqueue(Queue* queue, void* data) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (!newNode) {
        perror("Failed to enqueue");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

void* dequeue(Queue* queue) {
    if (queue->front == NULL) {
        return NULL;
    }

    QueueNode* temp = queue->front;
    void* data = temp->data;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(temp);
    return data;
}

void* peek(Queue* queue) {
    if (queue->front == NULL) return NULL;
    return queue->front->data;
}

bool isEmpty(Queue* queue) {
    return queue->front == NULL;
}

// get size of the queue
int getQueueSize(Queue* queue) {
    int size = 0;
    QueueNode* current = queue->front;
    while (current) {
        size++;
        current = current->next;
    }
    return size;
}

void destroyQueue(Queue* queue) {
    while (!isEmpty(queue)) {
        dequeue(queue);
    }
    free(queue);
}

void removeNode(Queue *q, QueueNode *minNode)
{
    // Handle case where the queue is empty
    if (q == NULL || q->front == NULL || minNode == NULL)
    {
        return;
    }

    // Special case: if the node to be removed is the front node
    if (q->front == minNode)
    {
        q->front = minNode->next;
        // If the node being removed is also the last node
        if (q->rear == minNode)
        {
            q->rear = NULL; // The queue is now empty
        }
    }
    else
    {
        // Traverse the queue to find the node just before the one to be removed
        QueueNode *prev = q->front;
        while (prev != NULL && prev->next != minNode)
        {
            prev = prev->next;
        }

        // If minNode is found in the queue, remove it
        if (prev != NULL)
        {
            prev->next = minNode->next;
            // If minNode is the last node, update the rear pointer
            if (q->rear == minNode)
            {
                q->rear = prev;
            }
        }
    }

    // Free the memory allocated for the node
    free(minNode);
}

