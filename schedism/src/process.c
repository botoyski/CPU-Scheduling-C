#include "process.h"

// process.c - Process structure and utility functions
void process_reset_all(Process p[], int n)
{
    // if process array is NULL or size is non-positive, nothing to reset, so return early
    if (p == 0 || n <= 0)
        return;

    // reset each process's metrics and state to initial values. We will set remaining_time to burst_time, start_time and finish_time to -1 (indicating not started/not finished), response_time to -1 (indicating not responded), turnaround_time and waiting_time to 0, started flag to 0, priority to 0, and time_in_queue to 0.
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
