/*
 * queue.c
 * Implements a circular FIFO queue.
 * Used heavily in Round Robin and MLFQ scheduling.
 */

#include <stdlib.h>

#include "queue.h"

/*
 * Initializes a queue with given capacity.
 * Returns 1 if success, 0 if invalid capacity.
 */

// queue_init initializes an IntQueue structure with a specified capacity. It allocates memory for the queue's data array and sets the initial values for head, tail, and size. It returns 1 on successful initialization and 0 if the provided capacity is invalid or if memory allocation fails.
int queue_init(IntQueue *q, int capacity)
{
    // if the queue pointer is NULL or capacity is non-positive, we cannot initialize the queue, so return failure
    if (q == NULL || capacity <= 0)
    {
        return 0;
    }

    // allocate memory for the queue's data array based on the specified capacity, and check for allocation failure. If allocation fails, return failure.
    q->data = (int *)malloc((size_t)capacity * sizeof(int));
    if (q->data == NULL)
    {
        return 0;
    }
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    return 1;
}

// free allocated memory for the queue
void queue_free(IntQueue *q)
{
    free(q->data);
}

// checks if the queue is empty
int queue_empty(const IntQueue *q)
{
    return q->size == 0;
}

//checks if the queue is full
int queue_full(const IntQueue *q)
{
    return q->size == q->capacity;
}

// adds element to the queue (FIFO)
int queue_push(IntQueue *q, int value)
{
    if (queue_full(q))
    {
        return 0;
    }
    // move tail forward(circular)
    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return 1;
}
// removes element from the front of the queue and stores it in value
int queue_pop(IntQueue *q, int *value)
{
    if (queue_empty(q))
    {
        return 0;
    }
    *value = q->data[q->head];// front of the queue
    // move front forward(circular)
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return 1;
}
