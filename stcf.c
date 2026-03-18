#include <stdio.h>
#include <limits.h>
#include "scheduler.h"
#include "trace.h"

/*
Shortest Time to Completion First
Preemptive version of SJF
Every time unit find process with smallest remaining time

At every clock tick, scheduler checks which process has smallest remaining CPU time
*/

void schedule_stcf(Process p[], int n)
{
    trace_reset();

    int time = 0;
    int completed = 0;
    int current_pid = -1;
    int segment_start = -1;
    int context_switches = 0;

    while (completed < n)
    {
        int idx = -1;
        int min_remaining = INT_MAX;

        /* find shortest remaining job */
        for (int i = 0; i < n; i++)
        {
            if (p[i].arrival_time <= time &&
                p[i].remaining_time > 0)
            {
                if (p[i].remaining_time < min_remaining)
                {
                    min_remaining = p[i].remaining_time;
                    idx = i;
                }
            }
        }

        /* CPU idle */
        if (idx == -1)
        {
            if (current_pid != -1)
            {
                if (!trace_add_segment(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return;
                }
                current_pid = -1;
                segment_start = -1;
            }
            time++;
            continue;
        }

        if (current_pid != idx)
        {
            if (current_pid != -1)
            {
                if (!trace_add_segment(current_pid, segment_start, time))
                {
                    fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                    return;
                }
                context_switches++;
            }
            current_pid = idx;
            segment_start = time;
        }

        /* first execution */
        if (!p[idx].started)
        {
            p[idx].start_time = time;
            p[idx].response_time =
                time - p[idx].arrival_time;

            p[idx].started = 1;
        }

        /* execute 1 time unit */
        p[idx].remaining_time--;

        time++;

        /* process finished */
        if (p[idx].remaining_time == 0)
        {
            completed++;

            if (!trace_add_segment(idx, segment_start, time))
            {
                fprintf(stderr, "Error: memory allocation failed in STCF trace.\n");
                return;
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

    trace_set_context_switches(context_switches);
}