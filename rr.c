#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"

typedef struct {
    int *data;
    int capacity;
    int head;
    int tail;
    int size;
} IntQueue;

static int queue_init(IntQueue *q, int capacity)
{
    q->data = (int *)malloc((size_t)capacity * sizeof(int));
    if (q->data == NULL)
        return 0;

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    return 1;
}

static void queue_free(IntQueue *q)
{
    free(q->data);
}

static int queue_empty(const IntQueue *q)
{
    return q->size == 0;
}

static void queue_push(IntQueue *q, int value)
{
    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
}

static int queue_pop(IntQueue *q)
{
    int value = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return value;
}

static int ensure_segment_capacity(int **seg_pid, int **seg_start, int **seg_end, int *capacity, int needed)
{
    if (needed <= *capacity)
        return 1;

    int new_capacity = (*capacity <= 0) ? 16 : (*capacity * 2);
    while (new_capacity < needed)
        new_capacity *= 2;

    int *new_pid = (int *)realloc(*seg_pid, (size_t)new_capacity * sizeof(int));
    if (new_pid == NULL)
        return 0;
    *seg_pid = new_pid;

    int *new_start = (int *)realloc(*seg_start, (size_t)new_capacity * sizeof(int));
    if (new_start == NULL)
        return 0;
    *seg_start = new_start;

    int *new_end = (int *)realloc(*seg_end, (size_t)new_capacity * sizeof(int));
    if (new_end == NULL)
        return 0;
    *seg_end = new_end;

    *capacity = new_capacity;
    return 1;
}

void schedule_rr(Process p[], int n, int quantum)
{
    int completed = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    int *arrived = (int *)calloc((size_t)n, sizeof(int));

    IntQueue ready_queue;
    if (!queue_init(&ready_queue, n + 1))
    {
        free(arrived);
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        return;
    }

    int seg_capacity = 16;
    int *seg_pid = (int *)malloc((size_t)seg_capacity * sizeof(int));
    int *seg_start = (int *)malloc((size_t)seg_capacity * sizeof(int));
    int *seg_end = (int *)malloc((size_t)seg_capacity * sizeof(int));
    int seg_count = 0;

    if (arrived == NULL || seg_pid == NULL || seg_start == NULL || seg_end == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        free(arrived);
        queue_free(&ready_queue);
        free(seg_pid);
        free(seg_start);
        free(seg_end);
        return;
    }

    while (completed < n)
    {
        for (int i = 0; i < n; i++)
        {
            if (!arrived[i] && p[i].arrival_time <= time)
            {
                queue_push(&ready_queue, i);
                arrived[i] = 1;
            }
        }

        if (queue_empty(&ready_queue))
        {
            time++;
            continue;
        }

        int idx = queue_pop(&ready_queue);

        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time = time - p[idx].arrival_time;
            p[idx].started = 1;
        }

        int run_for = quantum;
        if (p[idx].remaining_time < run_for)
            run_for = p[idx].remaining_time;

        int segment_start = time;

        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            time++;

            for (int i = 0; i < n; i++)
            {
                if (!arrived[i] && p[i].arrival_time <= time)
                {
                    queue_push(&ready_queue, i);
                    arrived[i] = 1;
                }
            }

            if (p[idx].remaining_time == 0)
                break;
        }

        if (!ensure_segment_capacity(&seg_pid, &seg_start, &seg_end, &seg_capacity, seg_count + 1))
        {
            fprintf(stderr, "Error: memory allocation failed while recording RR Gantt chart.\n");
            free(arrived);
            queue_free(&ready_queue);
            free(seg_pid);
            free(seg_start);
            free(seg_end);
            return;
        }

        seg_pid[seg_count] = idx;
        seg_start[seg_count] = segment_start;
        seg_end[seg_count] = time;
        seg_count++;

        if (p[idx].remaining_time == 0)
        {
            completed++;
            p[idx].finish_time = time;
            p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
        }
        else
        {
            queue_push(&ready_queue, idx);
        }

        prev_pid = idx;
    }

    double avg_response = 0.0;
    for (int i = 0; i < n; i++)
        avg_response += p[i].response_time;
    avg_response /= n;

    printf("\nUsing time quantum q=%d\n", quantum);
    printf("\n=== Gantt Chart ===\n");
    for (int i = 0; i < seg_count; i++)
        printf("[%s-]", p[seg_pid[i]].pid);

    printf("\nTime:");
    for (int i = 0; i < seg_count; i++)
        printf(" %d", seg_start[i]);
    if (seg_count > 0)
        printf(" %d", seg_end[seg_count - 1]);
    printf("\n\n");

    printf("Total context switches: %d\n", context_switches);
    printf("Average response time: %.2f\n", avg_response);

    free(arrived);
    queue_free(&ready_queue);
    free(seg_pid);
    free(seg_start);
    free(seg_end);
}
