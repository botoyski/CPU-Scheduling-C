#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "trace.h"

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

int schedule_rr(SchedulerState *state, int quantum)
{
    Process *p = state->processes;
    int n = state->num_processes;

    trace_reset();

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
        return -1;
    }

    if (arrived == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        free(arrived);
        queue_free(&ready_queue);
        return -1;
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

        if (!trace_add_segment(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording RR Gantt chart.\n");
            free(arrived);
            queue_free(&ready_queue);
            return -1;
        }

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

    trace_set_context_switches(context_switches);
    state->current_time = time;

    free(arrived);
    queue_free(&ready_queue);
    return 0;
}
