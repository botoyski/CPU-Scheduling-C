#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int *data;
    int capacity;
    int head;
    int tail;
    int size;
} IntQueue;

int queue_init(IntQueue *q, int capacity);
void queue_free(IntQueue *q);
int queue_empty(const IntQueue *q);
int queue_full(const IntQueue *q);
int queue_push(IntQueue *q, int value);
int queue_pop(IntQueue *q, int *value);

#endif
