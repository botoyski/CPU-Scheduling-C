#include "process.h"

void process_reset_all(Process p[], int n)
{
    if (p == 0 || n <= 0)
        return;

    for (int i = 0; i < n; i++)
    {
        p[i].remaining_time = p[i].burst_time;
        p[i].start_time = -1;
        p[i].finish_time = -1;
        p[i].response_time = -1;
        p[i].turnaround_time = 0;
        p[i].waiting_time = 0;
        p[i].started = 0;
        p[i].priority = 0;
        p[i].time_in_queue = 0;
    }
}
