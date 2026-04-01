#include <stdio.h>
#include <limits.h>
#include "scheduler.h"
#include "gantt.h"

/*
Shortest Time to Completion First
Preemptive version of SJF
Every time unit find process with smallest remaining time

At every clock tick, scheduler checks which process has smallest remaining CPU time
*/

// schedules processes using Shortest Time to Completion First algorithm
int schedule_stcf(SchedulerState *state)
{
    // extract processes, number of processes, and tracing functions from the scheduler state. We will use the provided tracing functions if available, otherwise default to the ones from gantt.h.
    Process *p = state->processes;
    int n = state->num_processes;

    // use provided trace functions if available, otherwise default to gantt implementations
    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    // reset the trace at the beginning of scheduling
    trace_reset_fn();

    // initialize counters for completed processes and current time, as well as variables to track the currently running process and the start time of its execution segment. We will also track the number of context switches that occur during scheduling.
    int time = 0;
    int completed = 0;
    int current_pid = -1;
    int segment_start = -1;
    int context_switches = 0;

    // continue scheduling until all processes are completed
    while (completed < n)
    {
        int idx = -1;
        int min_remaining = INT_MAX;

        // find the index of the next process to schedule by looking for the process with the smallest remaining time among those that have arrived and still have remaining time. If no such process is found, idx will remain -1.
        /* find shortest remaining job */
        for (int i = 0; i < n; i++)
        {
            // if process i has arrived (arrival_time <= current time) and still has remaining time to execute (remaining_time > 0), check if it has the smallest remaining time seen so far
            if (p[i].arrival_time <= time &&
                p[i].remaining_time > 0)
            {
                // if this process has a smaller remaining time than the current minimum, update min_remaining and idx to select this process
                if (p[i].remaining_time < min_remaining)
                {
                    min_remaining = p[i].remaining_time;
                    idx = i;
                }
            }
        }

        // if no process is ready to be scheduled (idx == -1), advance time by 1 and continue to the next iteration to check again for ready processes
        /* CPU idle */
        if (idx == -1)
        {
            // if the CPU is idle and there was a previously running process, we will record the end of its execution segment in the trace using the provided tracing function, and reset the current_pid and segment_start variables to indicate that no process is currently running. Then we will advance time by 1 and continue to the next iteration to check for ready processes again.
            if (current_pid != -1)
            {
                // if there was a previously running process, record the end of its execution segment in the trace using the provided tracing function, and reset the current_pid and segment_start variables to indicate that no process is currently running. Then we will advance time by 1 and continue to the next iteration to check for ready processes again.
                if (!trace_add_segment_fn(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return -1;
                }
                current_pid = -1;
                segment_start = -1;
            }
            time++;
            continue;
        }

        // if the selected process is different from the currently running process, we have a context switch. We will record the end of the previous execution segment (if any) in the trace using the provided tracing function, increment the context switch count, and then update current_pid and segment_start to reflect the newly scheduled process and the start time of its execution segment.
        if (current_pid != idx)
        {
            // if there was a previously running process, record the end of its execution segment in the trace using the provided tracing function, and reset the current_pid and segment_start variables to indicate that no process is currently running. Then we will update current_pid and segment_start to reflect the newly scheduled process and the start time of its execution segment. We will also increment the context switch count since we are switching from one process to another.
            if (current_pid != -1)
            {
                // if there was a previously running process, record the end of its execution segment in the trace using the provided tracing function, and reset the current_pid and segment_start variables to indicate that no process is currently running. Then we will update current_pid and segment_start to reflect the newly scheduled process and the start time of its execution segment. We will also increment the context switch count since we are switching from one process to another.
                if (!trace_add_segment_fn(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return -1;
                }
                context_switches++;
            }
            current_pid = idx;
            segment_start = time;
        }

        // if this is the first time the selected process is scheduled, we will record its start time and response time based on the current time and its arrival time. We will also set its started flag to indicate that it has started execution.
        /* first execution */
        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time =
                time - p[idx].arrival_time;

            p[idx].started = 1;
        }

        // execute the selected process for 1 time unit by decrementing its remaining_time and advancing the current time by 1. After executing, we will check if the process has completed (remaining_time == 0). If it has completed, we will record the end of its execution segment in the trace using the provided tracing function, reset current_pid and segment_start to indicate that no process is currently running, and then record its finish time and calculate its turnaround time and waiting time based on its arrival time and burst time. We will also increment the completed count since this process has finished execution.
        /* execute 1 time unit */
        p[idx].remaining_time--;

        // advance time by 1 after executing the process for this time unit
        time++;

        /* process finished */
        if (p[idx].remaining_time == 0)
        {
            completed++;

            if (!trace_add_segment_fn(idx, segment_start, time))
            {
                fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                return -1;
            }
            current_pid = -1;
            segment_start = -1;

            p[idx].finish_time = time;

            p[idx].turnaround_time =
                p[idx].finish_time - p[idx].arrival_time;

            p[idx].waiting_time =
                p[idx].turnaround_time - p[idx].burst_time;
        }
    }

    trace_set_context_switches_fn(context_switches);
    state->current_time = time;
    return 0;
}