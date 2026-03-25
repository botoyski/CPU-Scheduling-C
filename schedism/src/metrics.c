#include <stdio.h>

#include "metrics.h"
#include "gantt.h"

void compute_metrics_summary(Process p[], int n, MetricsSummary *summary)
{
    if (summary == NULL)
        return;

    summary->avg_turnaround = 0.0;
    summary->avg_waiting = 0.0;
    summary->avg_response = 0.0;
    summary->context_switches = trace_get_context_switches();

    if (p == NULL || n <= 0)
        return;

    double avg_turnaround = 0.0;
    double avg_wait = 0.0;
    double avg_response = 0.0;
    int fallback_count = 0;

    for (int i = 0; i < n; i++)
    {
        int turnaround = p[i].turnaround_time;
        if (turnaround < 0)
        {
            if (p[i].finish_time >= p[i].arrival_time)
                turnaround = p[i].finish_time - p[i].arrival_time;
            else
            {
                turnaround = 0;
                fallback_count++;
            }
        }

        int waiting = p[i].waiting_time;
        if (waiting < 0)
        {
            waiting = turnaround - p[i].burst_time;
            if (waiting < 0)
            {
                waiting = 0;
                fallback_count++;
            }
        }

        int response = p[i].response_time;
        if (response < 0)
        {
            if (p[i].start_time >= p[i].arrival_time)
                response = p[i].start_time - p[i].arrival_time;
            else
            {
                response = 0;
                fallback_count++;
            }
        }

        avg_turnaround += turnaround;
        avg_wait += waiting;
        avg_response += response;
    }

    summary->avg_turnaround = avg_turnaround / n;
    summary->avg_waiting = avg_wait / n;
    summary->avg_response = avg_response / n;

    if (fallback_count > 0)
        fprintf(stderr, "Warning: metrics summary used fallback values for %d fields.\n", fallback_count);
}

void print_results(Process p[], int n)
{
    MetricsSummary summary;
    compute_metrics_summary(p, n, &summary);

    printf("\n=== Per-Process Metrics ===\n");

    for (int i = 0; i < n; i++)
    {
        printf("\nProcess %s:\n", p[i].pid);
        printf("  Arrival Time:     %d\n", p[i].arrival_time);
        printf("  Burst Time:       %d\n", p[i].burst_time);
        printf("  Finish Time:      %d\n", p[i].finish_time);
        printf("  Turnaround Time:  %d - %d = %d\n",
               p[i].finish_time,
               p[i].arrival_time,
               p[i].turnaround_time);
        printf("  Waiting Time:     %d - %d = %d\n",
               p[i].turnaround_time,
               p[i].burst_time,
               p[i].waiting_time);
        printf("  Response Time:    %d - %d = %d\n",
               p[i].start_time,
               p[i].arrival_time,
               p[i].response_time);
    }

    printf("\n=== Averages ===\n");
    printf("Average Turnaround: %.2f\n", summary.avg_turnaround);
    printf("Average Waiting: %.2f\n", summary.avg_waiting);
    printf("Average Response: %.2f\n", summary.avg_response);
    printf("Context Switches: %d\n", summary.context_switches);

    print_gantt(p, n);
}
