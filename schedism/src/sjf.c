#include <stdio.h>
#include <limits.h>
#include "scheduler.h"
#include "gantt.h"

/*
Shortest Job First
Non-preemptive
Select job with smallest burst time among arrived processes

At each decision point, choose smallest burst among ready processes
*/

// schedules processes using Shortest Job First algorithm
int schedule_sjf(SchedulerState *state)
{
    // extract processes, number of processes, and tracing functions from the scheduler state. We will use the provided tracing functions if available, otherwise default to the ones from gantt.h.
    Process *p = state->processes;
    int n = state->num_processes;
    int verbose = state->verbose;

    // use provided trace functions if available, otherwise default to gantt implementations
    void (*trace_reset_fn)(void) = state->trace_reset_fn ? state->trace_reset_fn : trace_reset;
    int (*trace_add_segment_fn)(int, int, int) = state->trace_add_segment_fn ? state->trace_add_segment_fn : trace_add_segment;
    void (*trace_set_context_switches_fn)(int) = state->trace_set_context_switches_fn ? state->trace_set_context_switches_fn : trace_set_context_switches;

    // reset the trace at the beginning of scheduling
    trace_reset_fn();

    // initialize counters for completed processes and current time, and an array to track which processes have been visited/scheduled
    int completed = 0;
    int time = 0;

    // visited array to track which processes have been scheduled, initialized to 0 (not visited)
    int visited[n];

    // initialize visited array to 0 (not visited) for all processes
    for (int i = 0; i < n; i++)
        visited[i] = 0;

    // continue scheduling until all processes are completed
    while (completed < n)
    {
        int idx = -1;
        int min_burst = INT_MAX;

        // find the index of the next process to schedule by looking for the process with the smallest burst time among those that have arrived and have not been visited/scheduled yet. If no such process is found, idx will remain -1.
        /* find shortest available job */
        for (int i = 0; i < n; i++)
        {
            // if process i has arrived (arrival_time <= current time) and has not been visited/scheduled yet, check if it has the smallest burst time seen so far
            if (p[i].arrival_time <= time && !visited[i])
            {
                // if this process has a smaller burst time than the current minimum, update min_burst and idx to select this process
                if (p[i].burst_time < min_burst)
                {
                    min_burst = p[i].burst_time;
                    idx = i;
                }
            }
        }

        // if no process is ready to be scheduled (idx == -1), advance time by 1 and continue to the next iteration to check again for ready processes
        /* if no job available */
        if (idx == -1)
        {
            time++;
            continue;
        }

        // schedule the selected process at index idx. We will record its start time, execute it for its burst time, and then record its finish time and calculate its turnaround time, waiting time, and response time. We will also mark it as visited/scheduled and increment the completed count. Finally, we will record the execution segment in the trace using the provided tracing function.
        p[idx].start_time = time;
        int start = time;
        if (verbose)
            printf("t=%d: Process %s selected (SJF, burst=%d)\n", time, p[idx].pid, p[idx].burst_time);

        // execute the process for its burst time by advancing the current time by the burst time of the selected process. Since SJF is non-preemptive, we will run the process to completion before scheduling another one.
        time += p[idx].burst_time;
        if (!trace_add_segment_fn(idx, start, time))
        {
            fprintf(stderr, "Error: memory allocation failed in SJF trace.\n");
            return -1;
        }

        // after running the process to completion, we will record its finish time and calculate its turnaround time, waiting time, and response time based on its arrival time and burst time. We will also mark it as visited/scheduled and increment the completed count.
        p[idx].finish_time = time;

        p[idx].turnaround_time =
            p[idx].finish_time - p[idx].arrival_time;

        p[idx].waiting_time =
            p[idx].turnaround_time - p[idx].burst_time;

        p[idx].response_time =
            p[idx].start_time - p[idx].arrival_time;
        if (verbose)
            printf("t=%d: Process %s completes\n", time, p[idx].pid);

        visited[idx] = 1;
        completed++;
    }

    // SJF is non-preemptive, so context switches = 0
    trace_set_context_switches_fn(0);
    state->current_time = time;
    return 0;
}