
/*
 * metrics.c
 * Computes performance metrics for scheduling algorithms.
 *
 * Metrics:
 *   - Turnaround Time (TT)
 *   - Waiting Time (WT)
 *   - Response Time (RT)
 *
 * Also computes averages across all processes.
 */

#include <stdio.h>

#include "metrics.h"
#include "gantt.h"

/*
 * Computes average metrics from process array.
 *
 * Handles:
 *   - Missing values (sentinel = -1)
 *   - Derives missing metrics if possible
 */

// compute_metrics_summary calculates average turnaround time, waiting time, response time, and context switches from an array of Process structures. It handles missing values (indicated by -1) by attempting to derive them from other available metrics. The results are stored in a MetricsSummary structure pointed to by the summary parameter.
void compute_metrics_summary(Process p[], int n, MetricsSummary *summary)
{
    // if summary pointer is NULL, cannot store results, so return early
    if (summary == NULL)
        return;

    summary->avg_turnaround = 0.0;
    summary->avg_waiting = 0.0;
    summary->avg_response = 0.0;
    summary->context_switches = trace_get_context_switches();

    // if process array is NULL or empty, cannot compute metrics, so return early with averages as 0
    if (p == NULL || n <= 0)
        return;

    // compute sums for each metric, using fallback calculations if original values are missing (negative)
    double avg_turnaround = 0.0;
    double avg_wait = 0.0;
    double avg_response = 0.0;
    int fallback_count = 0;

    // iterate through each process to compute metrics
    for (int i = 0; i < n; i++)
    {

        // turnaround time
        int turnaround = p[i].turnaround_time;
        if (turnaround < 0)
        {
            // if finish_time and arrival_time are valid, derive turnaround time; otherwise, set to 0 and count as fallback
            if (p[i].finish_time >= p[i].arrival_time)
                turnaround = p[i].finish_time - p[i].arrival_time;
            else
            {
                turnaround = 0;
                fallback_count++;
            }
        }

        // waiting time
        int waiting = p[i].waiting_time;
        if (waiting < 0)
        {
            // if turnaround_time and burst_time are valid, derive waiting time; otherwise, set to 0 and count as fallback
            waiting = turnaround - p[i].burst_time;
            if (waiting < 0)
            {
                waiting = 0;
                fallback_count++;
            }
        }

        // response time
        int response = p[i].response_time;
        if (response < 0)
        {
            // if start_time and arrival_time are valid, derive response time; otherwise, set to 0 and count as fallback
            if (p[i].start_time >= p[i].arrival_time)
                response = p[i].start_time - p[i].arrival_time;
            else
            {
                response = 0;
                fallback_count++;
            }
        }

        // accumulate sums for averages
        avg_turnaround += turnaround;
        avg_wait += waiting;
        avg_response += response;
    }

    // compute averages
    summary->avg_turnaround = avg_turnaround / n;
    summary->avg_waiting = avg_wait / n;
    summary->avg_response = avg_response / n;

    // if any metrics were missing and had to be derived or defaulted, print a warning
    if (fallback_count > 0)
        fprintf(stderr, "Warning: metrics summary used fallback values for %d fields.\n", fallback_count);
}

// print_results displays the performance metrics for each process and the averages, as well as the Gantt chart. It takes an array of Process structures and its size as input. It computes the metrics summary, prints per-process metrics with explanations, prints average metrics, and then calls print_gantt to display the Gantt chart.
void print_results(Process p[], int n)
{
    // compute metrics summary for averages and context switches
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
