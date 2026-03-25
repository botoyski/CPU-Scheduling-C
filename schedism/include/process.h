#ifndef PROCESS_H
#define PROCESS_H

typedef struct {
    char pid[16];           // Process ID label

    int arrival_time;       // Time process enters system
    int burst_time;         // Total CPU time required

    int remaining_time;     // Remaining execution time

    int start_time;         // First time scheduled
    int finish_time;        // Completion time

    int response_time;      // start_time - arrival_time
    int turnaround_time;    // finish_time - arrival_time
    int waiting_time;       // turnaround_time - burst_time

    int started;            // flag if process has started execution

    int priority;           // current queue level (MLFQ, 0 = highest)
    int time_in_queue;      // CPU time consumed at current level (MLFQ)

} Process;

void process_reset_all(Process p[], int n);

#endif
