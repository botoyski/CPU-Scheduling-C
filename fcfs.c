#include <stdio.h>
#include "scheduler.h"

/*
FCFS Scheduling
First process that arrives gets CPU first
Non-preemptive

executes: arrival order → run until completion
*/

void schedule_fcfs(Process p[], int n)
{
    int completed = 0;
    int time = 0;
    int done[n];

    for (int i = 0; i < n; i++)
        done[i] = 0;

    while (completed < n)
    {
        int idx = -1;
        int min_arrival = 2147483647;

        for (int i = 0; i < n; i++)
        {
            if (!done[i] && p[i].arrival_time <= time)
            {
                if (p[i].arrival_time < min_arrival)
                {
                    min_arrival = p[i].arrival_time;
                    idx = i;
                }
            }
        }

        if (idx == -1)
        {
            int next_arrival = 2147483647;
            for (int i = 0; i < n; i++)
            {
                if (!done[i] && p[i].arrival_time < next_arrival)
                    next_arrival = p[i].arrival_time;
            }
            time = next_arrival;
            continue;
        }

        /* Process starts */
        p[idx].start_time = time;
        p[idx].response_time = p[idx].start_time - p[idx].arrival_time;
        p[idx].started = 1;

        /* Execute full burst */
        time += p[idx].burst_time;

        /* Process completes */
        p[idx].finish_time = time;

        /* Metrics */
        p[idx].turnaround_time = p[idx].finish_time - p[idx].arrival_time;
        p[idx].waiting_time = p[idx].turnaround_time - p[idx].burst_time;

        done[idx] = 1;
        completed++;
    }
}