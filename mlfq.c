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

static int highest_non_empty_queue(int levels, IntQueue queues[])
{
    for (int lvl = 0; lvl < levels; lvl++)
    {
        if (!queue_empty(&queues[lvl]))
            return lvl;
    }
    return -1;
}

static void enqueue_new_arrivals(Process p[], int n, int time, int arrived[], IntQueue *q0)
{
    for (int i = 0; i < n; i++)
    {
        if (!arrived[i] && p[i].arrival_time <= time)
        {
            p[i].priority = 0;
            p[i].time_in_queue = 0;
            queue_push(q0, i);
            arrived[i] = 1;
            if (!trace_is_quiet())
                printf("t=%d: Process %s enters Q0\n", p[i].arrival_time, p[i].pid);
        }
    }
}

static void do_priority_boost(Process p[], int n, int completed[], int levels, IntQueue queues[], int time)
{
    for (int lvl = 1; lvl < levels; lvl++)
    {
        while (!queue_empty(&queues[lvl]))
        {
            int idx = queue_pop(&queues[lvl]);
            if (!completed[idx])
            {
                p[idx].priority = 0;
                p[idx].time_in_queue = 0;
                queue_push(&queues[0], idx);
            }
        }
    }

    /* Keep queue 0 jobs but refresh their queue/accounting state. */
    int q0_size = queues[0].size;
    for (int i = 0; i < q0_size; i++)
    {
        int idx = queue_pop(&queues[0]);
        if (!completed[idx])
        {
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            queue_push(&queues[0], idx);
        }
    }

    if (!trace_is_quiet())
        printf("t=%d: Priority boost: all ready processes -> Q0\n", time);
}

int schedule_mlfq(SchedulerState *state, MLFQConfig *config)
{
    Process *p = state->processes;
    int n = state->num_processes;

    trace_reset();

    int completed_count = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    int *arrived = (int *)calloc((size_t)n, sizeof(int));
    int *completed = (int *)calloc((size_t)n, sizeof(int));

    IntQueue *queues = (IntQueue *)malloc((size_t)config->levels * sizeof(IntQueue));
    if (arrived == NULL || completed == NULL || queues == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed in MLFQ scheduler.\n");
        free(arrived);
        free(completed);
        free(queues);
        return -1;
    }

    for (int lvl = 0; lvl < config->levels; lvl++)
    {
        if (!queue_init(&queues[lvl], n + 1))
        {
            fprintf(stderr, "Error: memory allocation failed in MLFQ queue setup.\n");
            for (int i = 0; i < lvl; i++)
                queue_free(&queues[i]);
            free(arrived);
            free(completed);
            free(queues);
            return -1;
        }
    }

    int next_boost = config->boost_period;

    if (!trace_is_quiet())
    {
        printf("\n=== MLFQ Configuration ===\n");
        for (int lvl = 0; lvl < config->levels; lvl++)
        {
            if (config->quantum[lvl] == -1)
                printf("Queue %d: FCFS", lvl);
            else
                printf("Queue %d: q=%d", lvl, config->quantum[lvl]);

            if (config->allotment[lvl] != -1)
                printf(", allotment=%d", config->allotment[lvl]);

            if (lvl == 0)
                printf(" (highest priority)");
            if (lvl == config->levels - 1)
                printf(" (lowest priority)");

            printf("\n");
        }
        printf("Boost period: %d\n", config->boost_period);
        printf("\n=== Execution Trace ===\n");
    }

    while (completed_count < n)
    {
        enqueue_new_arrivals(p, n, time, arrived, &queues[0]);

        int level = highest_non_empty_queue(config->levels, queues);
        if (level == -1)
        {
            time++;
            if (config->boost_period > 0 && time == next_boost)
            {
                do_priority_boost(p, n, completed, config->levels, queues, time);
                next_boost += config->boost_period;
            }
            continue;
        }

        int idx = queue_pop(&queues[level]);
        if (completed[idx])
            continue;

        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time = time - p[idx].arrival_time;
            p[idx].started = 1;
        }

        if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
        {
            if (level < config->levels - 1)
            {
                p[idx].priority = level + 1;
                p[idx].time_in_queue = 0;
                queue_push(&queues[level + 1], idx);
                  if (!trace_is_quiet())
                      printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                          time,
                          p[idx].pid,
                          level + 1,
                          level);
            }
            else
            {
                queue_push(&queues[level], idx);
            }
            prev_pid = idx;
            continue;
        }

        int run_for;
        if (config->quantum[level] == -1)
            run_for = p[idx].remaining_time;
        else
            run_for = config->quantum[level] < p[idx].remaining_time ? config->quantum[level] : p[idx].remaining_time;

        if (config->allotment[level] != -1)
        {
            int remaining_allot = config->allotment[level] - p[idx].time_in_queue;
            if (remaining_allot < run_for)
                run_for = remaining_allot;
        }

        if (config->boost_period > 0)
        {
            int to_boost = next_boost - time;
            if (to_boost > 0 && to_boost < run_for)
                run_for = to_boost;
        }

        if (run_for <= 0)
            run_for = 1;

        int segment_start = time;

        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            p[idx].time_in_queue++;
            time++;

            enqueue_new_arrivals(p, n, time, arrived, &queues[0]);

            if (p[idx].remaining_time == 0)
                break;

            if (config->boost_period > 0 && time == next_boost)
                break;
        }

        if (!trace_add_segment(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording MLFQ Gantt chart.\n");
            for (int lvl = 0; lvl < config->levels; lvl++)
                queue_free(&queues[lvl]);
            free(arrived);
            free(completed);
            free(queues);
            return -1;
        }

        if (p[idx].remaining_time == 0)
        {
            completed[idx] = 1;
            completed_count++;
            p[idx].finish_time = time;
            p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
            if (!trace_is_quiet())
                printf("t=%d: Process %s completes\n", time, p[idx].pid);
        }
        else if (config->boost_period > 0 && time == next_boost)
        {
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            queue_push(&queues[0], idx);
            do_priority_boost(p, n, completed, config->levels, queues, time);
            next_boost += config->boost_period;
        }
        else
        {
            int demoted = 0;
            if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
            {
                if (level < config->levels - 1)
                {
                    p[idx].priority = level + 1;
                    p[idx].time_in_queue = 0;
                    queue_push(&queues[level + 1], idx);
                    if (!trace_is_quiet())
                        printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                               time,
                               p[idx].pid,
                               level + 1,
                               level);
                    demoted = 1;
                }
            }

            if (!demoted)
                queue_push(&queues[level], idx);
        }

        prev_pid = idx;
    }

    trace_set_context_switches(context_switches);
    state->current_time = time;

    for (int lvl = 0; lvl < config->levels; lvl++)
        queue_free(&queues[lvl]);

    free(arrived);
    free(completed);
    free(queues);
    return 0;
}
