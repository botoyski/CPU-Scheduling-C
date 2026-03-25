#include <stdlib.h>

#include "queue.h"

int queue_init(IntQueue *q, int capacity)
{
    if (q == NULL || capacity <= 0)
    {
        return 0;
    }

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

void queue_free(IntQueue *q)
{
    free(q->data);
}

int queue_empty(const IntQueue *q)
{
    return q->size == 0;
}

int queue_full(const IntQueue *q)
{
    return q->size == q->capacity;
}

int queue_push(IntQueue *q, int value)
{
    if (queue_full(q))
    {
        return 0;
    }
    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return 1;
}

int queue_pop(IntQueue *q, int *value)
{
    if (queue_empty(q))
    {
        return 0;
    }
    *value = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return 1;
}
