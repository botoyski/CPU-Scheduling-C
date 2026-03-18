#include <stdio.h>
#include "process.h"

/* Print results */

void print_results(Process p[], int n)
{
    double avg_turnaround = 0;
    double avg_wait = 0;
    double avg_response = 0;

       printf("\nPID            AT   BT   CT   TAT   WT   RT\n");

    for (int i = 0; i < n; i++)
    {
              printf("%-14s %3d %4d %4d %5d %4d %4d\n",
               p[i].pid,
               p[i].arrival_time,
               p[i].burst_time,
               p[i].finish_time,
               p[i].turnaround_time,
               p[i].waiting_time,
               p[i].response_time);

        avg_turnaround += p[i].turnaround_time;
        avg_wait += p[i].waiting_time;
        avg_response += p[i].response_time;
    }

    printf("\nAverage Turnaround: %.2f",
           avg_turnaround / n);

    printf("\nAverage Waiting: %.2f",
           avg_wait / n);

    printf("\nAverage Response: %.2f\n",
           avg_response / n);
}