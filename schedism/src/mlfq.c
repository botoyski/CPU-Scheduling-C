#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "gantt.h"
#include "queue.h"

// Helper functions for MLFQ scheduling
static int highest_non_empty_queue(int levels, IntQueue queues[])
{
    // check from highest priority (lowest index) to lowest for a non-empty queue
    for (int lvl = 0; lvl < levels; lvl++)
    {
        if (!queue_empty(&queues[lvl]))
            return lvl;
    }
    return -1;
}

// cleanup_mlfq_resources frees all dynamically allocated resources used in MLFQ scheduling, including the queues and the arrived/completed arrays. It takes the array of queues, the number of levels, and pointers to the arrived and completed arrays as input.
static void cleanup_mlfq_resources(IntQueue queues[], int levels, int *arrived, int *completed)
{
    // free each queue in the queues array
    if (queues != NULL)
    {
        // free each queue and then free the queues array itself
        for (int lvl = 0; lvl < levels; lvl++)
            queue_free(&queues[lvl]);
        free(queues);
    }
    free(arrived);
    free(completed);
}

// enqueue_new_arrivals checks for processes that have arrived at the current time and enqueues them into the highest priority queue (Q0). It updates the arrived array to track which processes have been enqueued. It also prints a message for each new arrival if tracing is not quiet. It returns 1 on success and 0 on failure (e.g., queue overflow).
static int enqueue_new_arrivals(Process p[], int n, int time, int arrived[], IntQueue *q0, int (*trace_is_quiet_fn)(void))
{
    // check for newly arrived processes at current time and add to Q0
    for (int i = 0; i < n; i++)
    {
        // if process i arrives at this time, add to Q0
        if (!arrived[i] && p[i].arrival_time <= time)
        {
            // initialize priority and time_in_queue for new arrivals, then enqueue into Q0
            p[i].priority = 0;
            p[i].time_in_queue = 0;
            if (!queue_push(q0, i)) {
                fprintf(stderr, "Error: queue overflow in enqueue_new_arrivals.\n");
                return 0;
            }
            arrived[i] = 1;
            int quiet = trace_is_quiet_fn ? trace_is_quiet_fn() : trace_is_quiet();
            if (!quiet)
                printf("t=%d: Process %s enters Q0\n", p[i].arrival_time, p[i].pid);
        }
    }
    return 1;
}

// do_priority_boost performs a priority boost by moving all ready processes from lower priority queues to the highest priority queue (Q0). It resets their priority and time_in_queue, and prints a message if tracing is not quiet. It returns 1 on success and 0 on failure (e.g., queue overflow).
static int do_priority_boost(Process p[], int n, int completed[], int levels, IntQueue queues[], int time, int (*trace_is_quiet_fn)(void))
{
    // move all ready processes from lower priority queues to Q0, and reset their priority and time_in_queue. We will check each queue from lowest to highest (except Q0) and move processes that are not completed. After moving, we will also rotate the existing processes in Q0 to maintain their order. Finally, we will print a message about the boost if tracing is not quiet.
    for (int lvl = 1; lvl < levels; lvl++)
    {
        // for each queue level, we will pop all processes and if they are not completed, we will reset their priority and time_in_queue and push them to Q0
        while (!queue_empty(&queues[lvl]))
        {
            // pop the next process index from this queue
            int idx;
            if (!queue_pop(&queues[lvl], &idx)) {
                fprintf(stderr, "Error: queue underflow in MLFQ boost.\n");
                return 0;
            }
            // if this process is not completed, reset its priority and time_in_queue and push to Q0
            if (!completed[idx])
            {
                p[idx].priority = 0;
                p[idx].time_in_queue = 0;
                // if pushing to Q0 fails, print an error and return failure
                if (!queue_push(&queues[0], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                    return 0;
                }
            }
        }
    }

    /*
    Snapshot queue size so we only rotate jobs that were already in Q0
    before this boost. This avoids reprocessing entries that are enqueued
    during this loop.
    */
    // rotate existing Q0 processes to maintain their order after the boost
    int q0_size = queues[0].size;
    for (int i = 0; i < q0_size; i++)
    {
        // pop the next process index from Q0
        int idx;
        if (!queue_pop(&queues[0], &idx)) {
            fprintf(stderr, "Error: queue underflow in MLFQ boost.\n");
            return 0;
        }
        // push it back to Q0 to rotate it to the end of the queue
        if (!completed[idx])
        {
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            // if pushing back to Q0 fails, print an error and return failure
            if (!queue_push(&queues[0], idx)) {
                fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                return 0;
            }
        }
    }

    // print a message about the boost if tracing is not quiet
    int quiet = trace_is_quiet_fn ? trace_is_quiet_fn() : trace_is_quiet();
    if (!quiet)
        printf("t=%d: Priority boost: all ready processes -> Q0\n", time);

    return 1;
}

// schedule_mlfq implements the MLFQ scheduling algorithm. It takes a SchedulerState containing the processes and tracing functions, and an MLFQConfig containing the queue configurations. It manages multiple queues for different priority levels, handles process arrivals, scheduling, demotion, and priority boosts according to the MLFQ rules. It returns 0 on success and -1 on failure (e.g., invalid input, memory allocation failure).
int schedule_mlfq(SchedulerState *state, MLFQConfig *config)
{
    // validate input parameters
    if (state == NULL || config == NULL || state->processes == NULL)
    {
        fprintf(stderr, "Error: invalid scheduler state/config for MLFQ.\n");
        return -1;
    }

    // validate MLFQ config values
    if (config->levels <= 0 || config->levels > MLFQ_MAX_LEVELS)
    {
        fprintf(stderr, "Error: invalid MLFQ levels=%d.\n", config->levels);
        return -1;
    }

    // validate each queue level configuration
    if (config->boost_period <= 0)
    {
        fprintf(stderr, "Error: invalid MLFQ boost period=%d.\n", config->boost_period);
        return -1;
    }

    // initialize local variables for processes, number of processes, and tracing functions
    Process *p = state->processes;
    int n = state->num_processes;

    // use provided tracing functions or default to the ones from gantt.h
    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;
    int (*trace_is_quiet_fn)(void) = state->trace_is_quiet_fn ? state->trace_is_quiet_fn : trace_is_quiet;

    // reset the trace at the beginning of scheduling
    trace_reset_fn();

    // initialize data structures for MLFQ scheduling, including arrays to track arrived and completed processes, and the queues for each priority level. We will also initialize counters for completed processes, current time, context switches, and previous process index.
    int completed_count = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    // allocate memory for arrived and completed arrays, and the queues for each level. We will check for allocation failures and handle them by printing an error message and returning failure.
    int *arrived = (int *)calloc((size_t)n, sizeof(int));
    int *completed = (int *)calloc((size_t)n, sizeof(int));

    // allocate an array of queues for each MLFQ level
    IntQueue *queues = (IntQueue *)malloc((size_t)config->levels * sizeof(IntQueue));
    if (arrived == NULL || completed == NULL || queues == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed in MLFQ scheduler.\n");
        free(queues);
        free(arrived);
        free(completed);
        return -1;
    }

    // initialize each queue for the MLFQ levels, and check for allocation failures
    for (int lvl = 0; lvl < config->levels; lvl++)
    {
        // initialize the queue for this level with a capacity of n+1 (to avoid edge case of full queue when all processes are ready)
        if (!queue_init(&queues[lvl], n + 1))
        {
            fprintf(stderr, "Error: memory allocation failed in MLFQ queue setup.\n");
            cleanup_mlfq_resources(queues, lvl, arrived, completed);
            return -1;
        }
    }

    // main scheduling loop continues until all processes are completed. In each iteration, we will check for new arrivals, select the next process to run based on the highest non-empty queue, handle scheduling and demotion according to the MLFQ rules, and perform priority boosts when the boost period is reached.
    int next_boost = config->boost_period;

    // print the MLFQ configuration and initial message if tracing is not quiet
    if (!trace_is_quiet_fn())
    {
        // print the MLFQ configuration details, including the quantum and allotment for each queue level, and the boost period. This helps to understand the scheduling behavior before we print the execution trace.
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

    // scheduling loop
    while (completed_count < n)
    {
        // check for newly arrived processes at current time and add to Q0
        if (!enqueue_new_arrivals(p, n, time, arrived, &queues[0], trace_is_quiet_fn))
        {
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }

        // select the next process to run from the highest non-empty queue
        int level = highest_non_empty_queue(config->levels, queues);
        if (level == -1)
        {
            // if no processes are ready, advance time to the next boost or next arrival, whichever is sooner
            time++;
            if (config->boost_period > 0 && time == next_boost)
            {
                // perform priority boost when the boost period is reached, and then update the next_boost time
                if (!do_priority_boost(p, n, completed, config->levels, queues, time, trace_is_quiet_fn))
                {
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
                next_boost += config->boost_period;
            }
            continue;
        }

        // pop the next process index from the selected queue
        int idx;
        if (!queue_pop(&queues[level], &idx)) {
            fprintf(stderr, "Error: queue underflow in MLFQ main loop.\n");
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }
        // if this process is different from the previously running process, increment context switches
        if (completed[idx])
            continue;

        // if the previously running process is different from the current one, we have a context switch
        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        // record start time and response time if first time scheduled
        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time = time - p[idx].arrival_time;
            p[idx].started = 1;
        }

        // check if the process has exhausted its allotment at this level and needs to be demoted before running
        if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
        {
            // if this process has exhausted its allotment, demote it to the next lower queue (if not already at lowest) and reset its time_in_queue. Then continue to the next iteration to select another process to run. We will also print a message about the demotion if tracing is not quiet.
            if (level < config->levels - 1)
            {
                // demote to next lower queue
                p[idx].priority = level + 1;
                p[idx].time_in_queue = 0;
                if (!queue_push(&queues[level + 1], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ demotion.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
                // print a message about the demotion if tracing is not quiet
                if (!trace_is_quiet_fn())
                    printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                        time,
                        p[idx].pid,
                        level + 1,
                        level);
            }
            // if already at lowest queue, just re-enqueue it there
            else
            {
                // re-enqueue in the same queue if already at lowest level
                if (!queue_push(&queues[level], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
            }
            prev_pid = idx;
            continue;
        }

        // determine how long to run this process based on the quantum for this level, the remaining time of the process, and the remaining allotment if applicable. We will also check if a priority boost is scheduled to occur during this time slice, and if so, we will only run until the boost time to ensure the boost happens on time.
        int run_for;
        if (config->quantum[level] == -1)
            run_for = p[idx].remaining_time;
        else
            run_for = config->quantum[level] < p[idx].remaining_time ? config->quantum[level] : p[idx].remaining_time;

        // if there is an allotment for this level, we need to check how much time the process has already consumed at this level and adjust run_for accordingly to not exceed the allotment
        if (config->allotment[level] != -1)
        {
            // calculate the remaining allotment for this process at the current level and adjust run_for if it exceeds the remaining allotment
            int remaining_allot = config->allotment[level] - p[idx].time_in_queue;
            if (remaining_allot < run_for)
                run_for = remaining_allot;
        }

        // if there is a boost period, we need to check if the next boost is scheduled to occur during this run and adjust run_for to ensure we stop before the boost to allow it to happen on time
        if (config->boost_period > 0)
        {
            // calculate the time until the next boost and adjust run_for if it exceeds that time
            int to_boost = next_boost - time;
            if (to_boost > 0 && to_boost < run_for)
                run_for = to_boost;
        }

        // if run_for is 0 or negative due to any of the above calculations, we will set it to 1 to ensure the process gets CPU time and we can make progress (e.g., to trigger a boost or demotion in the next iteration)
        if (run_for <= 0)
            run_for = 1;

        // run the process for the determined time slice, updating its remaining time and time_in_queue, and advancing the global time. We will also check for new arrivals at each tick during this run and enqueue them into Q0. If the process finishes during this run, we will break early to record the correct finish time.
        int segment_start = time;

        // run the process for the calculated run_for time, checking for new arrivals at each tick and handling them accordingly. We will also check if the process finishes during this run to break early and record the correct finish time.
        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            p[idx].time_in_queue++;
            time++;

            // check for newly arrived processes at each tick and add them to Q0
            if (!enqueue_new_arrivals(p, n, time, arrived, &queues[0], trace_is_quiet_fn))
            {
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }

            // if the process finishes during this tick, break early to record the correct finish time
            if (p[idx].remaining_time == 0)
                break;

            // if a boost is scheduled to occur during this run, we will break early to allow the boost to happen on time
            if (config->boost_period > 0 && time == next_boost)
                break;
        }

        // record the execution segment for the Gantt chart using the provided tracing function, and check for memory allocation failure
        if (!trace_add_segment_fn(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording MLFQ Gantt chart.\n");
            cleanup_mlfq_resources(queues, config->levels, arrived, completed);
            return -1;
        }

        // after running, check if the process has completed. If so, record its finish time and metrics, and mark it as completed. If not, we will check if it needs to be demoted due to exhausting its allotment, or if it should be re-enqueued in the same queue. We will also handle priority boosts if the boost period is reached during this time.
        if (p[idx].remaining_time == 0)
        {
            completed[idx] = 1;
            completed_count++;
            p[idx].finish_time = time;
            p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
            p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;
            // print a message about the process completion if tracing is not quiet
            if (!trace_is_quiet_fn())
                printf("t=%d: Process %s completes\n", time, p[idx].pid);
        }
        // if not completed, check if it needs to be demoted due to exhausting its allotment, or if it should be re-enqueued in the same queue. We will also handle priority boosts if the boost period is reached during this time.
        else if (config->boost_period > 0 && time == next_boost)
        {
            // if a boost is scheduled to occur at this time, we will perform the boost before re-enqueuing the process. This ensures that the boost happens on time and affects the scheduling of processes correctly. After the boost, we will reset the priority and time_in_queue of this process and enqueue it into Q0 to reflect the boost.
            p[idx].priority = 0;
            p[idx].time_in_queue = 0;
            if (!queue_push(&queues[0], idx)) {
                fprintf(stderr, "Error: queue overflow in MLFQ boost.\n");
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }
            // perform priority boost when the boost period is reached, and then update the next_boost time
            if (!do_priority_boost(p, n, completed, config->levels, queues, time, trace_is_quiet_fn))
            {
                cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                return -1;
            }
            next_boost += config->boost_period;
        }
        // if the process is not completed and did not get boosted, check if it needs to be demoted due to exhausting its allotment, or if it should be re-enqueued in the same queue
        else
        {
            // check if the process has exhausted its allotment at this level and needs to be demoted before re-enqueuing. If it has exhausted its allotment, we will demote it to the next lower queue (if not already at lowest) and reset its time_in_queue. If it has not exhausted its allotment, we will simply re-enqueue it in the same queue. We will also print a message about the demotion if tracing is not quiet.
            int demoted = 0;
            if (config->allotment[level] != -1 && p[idx].time_in_queue >= config->allotment[level])
            {
                // if this process has exhausted its allotment, demote it to the next lower queue (if not already at lowest) and reset its time_in_queue. Then continue to the next iteration to select another process to run. We will also print a message about the demotion if tracing is not quiet.
                if (level < config->levels - 1)
                {
                    // demote to next lower queue
                    p[idx].priority = level + 1;
                    p[idx].time_in_queue = 0;
                    if (!queue_push(&queues[level + 1], idx)) {
                        fprintf(stderr, "Error: queue overflow in MLFQ demotion.\n");
                        cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                        return -1;
                    }
                    // print a message about the demotion if tracing is not quiet
                    if (!trace_is_quiet_fn())
                        printf("t=%d: Process %s -> Q%d (exhausted Q%d allotment)\n",
                               time,
                               p[idx].pid,
                               level + 1,
                               level);
                    demoted = 1;
                }
            }

            // if not demoted and not completed, re-enqueue in the same queue
            if (!demoted) {
                if (!queue_push(&queues[level], idx)) {
                    fprintf(stderr, "Error: queue overflow in MLFQ.\n");
                    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
                    return -1;
                }
            }
        }

        prev_pid = idx;
    }

    // after the main scheduling loop, we will set the total context switches in the trace using the provided tracing function, and update the current time in the scheduler state. Finally, we will clean up all dynamically allocated resources used for MLFQ scheduling before returning success.
    trace_set_context_switches_fn(context_switches);
    state->current_time = time;

    // clean up all dynamically allocated resources used for MLFQ scheduling before returning success
    cleanup_mlfq_resources(queues, config->levels, arrived, completed);
    return 0;
}
