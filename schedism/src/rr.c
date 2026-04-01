#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "gantt.h"
#include "queue.h"

/*
 * rr.c
 * Implements Round Robin scheduling.
 *
 * Key Idea:
 *   - Each process gets fixed time quantum
 *   - If not finished → goes back to queue
 */

// schedules processes using Round Robin algorithm with given time quantum
int schedule_rr(SchedulerState *state, int quantum)
{
    Process *p = state->processes;
    int n = state->num_processes;
    int verbose = scheduler_is_verbose(state);
    SchedulerTraceOps trace;
    scheduler_get_trace_ops(state, &trace);

    trace.reset();

    int completed = 0;
    int time = 0;
    int context_switches = 0;
    int prev_pid = -1;

    int result = -1;
    int *arrived = (int *)calloc((size_t)n, sizeof(int));

    // check for memory allocation failure
    if (arrived == NULL) {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        goto cleanup;
    }

    // initialize ready queue
    IntQueue ready_queue;
    int queue_initialized = 0;
    // +1 to capacity to avoid edge case of full queue when all processes are ready
    if (!queue_init(&ready_queue, n + 1))
    {
        fprintf(stderr, "Error: memory allocation failed in RR scheduler.\n");
        goto cleanup;
    }
    queue_initialized = 1;

    // continue until all processes are completed
    while (completed < n)
    {
        // check for newly arrived processes at current time and add to ready queue
        for (int i = 0; i < n; i++)
        {
            // if process i arrives at this time, add to ready queue
            if (!arrived[i] && p[i].arrival_time <= time)
            {
                // if new process arrives while current is running, it will be scheduled in the next round
                if (!queue_push(&ready_queue, i)) {
                    fprintf(stderr, "Error: queue overflow in RR scheduler.\n");
                    goto cleanup;
                }
                arrived[i] = 1;
                if (verbose)
                    printf("t=%d: Process %s arrives and joins RR queue\n", time, p[i].pid);
            }
        }

        // if no processes are ready, advance time
        if (queue_empty(&ready_queue))
        {
            time++;
            continue;
        }

        // get next process from the ready queue
        int idx;
        if (!queue_pop(&ready_queue, &idx)) {
            fprintf(stderr, "Error: queue underflow in RR scheduler.\n");
            goto cleanup;
        }

        // context switch if different process is scheduled
        if (prev_pid != -1 && prev_pid != idx)
            context_switches++;

        // record start time and response time if first time scheduled
        if (!p[idx].started)
        {
            scheduler_mark_process_started(&p[idx], time);
        }

        // run process for a time quantum or until completion
        int run_for = quantum;
        if (p[idx].remaining_time < run_for)
            run_for = p[idx].remaining_time;

        // record execution segment for Gantt chart
        int segment_start = time;
        if (verbose)
            printf("t=%d: Process %s runs for up to %d units\n", time, p[idx].pid, run_for);

        // run process for its time quantum (or until completion)
        for (int tick = 0; tick < run_for; tick++)
        {
            p[idx].remaining_time--;
            time++;

            // if process finishes during this tick, break early to record segment end time
            if (p[idx].remaining_time == 0)
                break;
        }

        // record execution segment for Gantt chart
        if (!trace.add_segment(idx, segment_start, time))
        {
            fprintf(stderr, "Error: memory allocation failed while recording RR Gantt chart.\n");
            goto cleanup;
        }

        // if process completed, record finish time and metrics; otherwise, re-queue it
        if (p[idx].remaining_time == 0)
        {
            completed++;
            scheduler_mark_process_completed(&p[idx], time);
            if (verbose)
                printf("t=%d: Process %s completes\n", time, p[idx].pid);
        }
        // if not completed, add back to ready queue
        else
        {
            // if process still has time left, add back to ready queue
            if (!queue_push(&ready_queue, idx)) {
                fprintf(stderr, "Error: queue overflow in RR scheduler.\n");
                goto cleanup;
            }
            if (verbose)
                printf("t=%d: Process %s re-queued (remaining=%d)\n", time, p[idx].pid, p[idx].remaining_time);
        }

        prev_pid = idx;
    }

    // set total context switches in trace
    trace.set_context_switches(context_switches);
    state->current_time = time;
    result = 0;

cleanup: // cleanup resources and return result
    free(arrived);
    if (queue_initialized)
        queue_free(&ready_queue);
    return result;
}
