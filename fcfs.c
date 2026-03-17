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
    int time = 0;

    for (int i = 0; i < n; i++)
    {
        /* If CPU idle, jump to arrival */
        if (time < p[i].arrival_time)
            time = p[i].arrival_time;

        /* Process starts */
        p[i].start_time = time;

        /* Execute full burst */
        time += p[i].burst_time;

        /* Process completes */
        p[i].finish_time = time;

        /* Metrics */
        p[i].turnaround_time = p[i].finish_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
        p[i].response_time = p[i].start_time - p[i].arrival_time;
    }
}