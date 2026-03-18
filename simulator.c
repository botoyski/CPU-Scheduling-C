#include <stdio.h>
#include <ctype.h>

#include "process.h"
#include "scheduler.h"
#include "trace.h"

static char pid_symbol(const Process *p)
{
       unsigned char c = (unsigned char)p->pid[0];
       if (isalnum(c))
              return (char)c;
       return '#';
}

static void print_regular_markers(int total_time, int step)
{
       printf("Time markers (every %d):", step);
       for (int t = 0; t <= total_time; t += step)
              printf(" %d", t);
       if (total_time % step != 0)
              printf(" %d", total_time);
       printf("\n");
}

static void print_gantt(Process p[], int n)
{
       int total_time = trace_total_time();
       if (total_time <= 0)
              return;

       (void)n;

       printf("\n=== Gantt Chart (1 unit = 1 char) ===\n");
       printf("CPU : ");
       for (int t = 0; t < total_time; t++)
       {
              int idx = trace_pid_at_time(t);
              if (idx < 0)
                     putchar('.');
              else
                     putchar(pid_symbol(&p[idx]));
       }
       printf("\n");
       print_regular_markers(total_time, 10);

       if (total_time > 80)
       {
              const int scale = 10;
              printf("\n=== Scaled Gantt Chart ===\n");
              printf("Each char = %d time units\n", scale);
              printf("CPU : ");
              for (int t = 0; t < total_time; t += scale)
              {
                     int idx = trace_pid_at_time(t);
                     if (idx < 0)
                            putchar('.');
                     else
                            putchar(pid_symbol(&p[idx]));
              }
              printf("\n");
              print_regular_markers(total_time, 50);
       }
}

void compute_metrics_summary(Process p[], int n, MetricsSummary *summary)
{
       double avg_turnaround = 0.0;
       double avg_wait = 0.0;
       double avg_response = 0.0;

       for (int i = 0; i < n; i++)
       {
              avg_turnaround += p[i].turnaround_time;
              avg_wait += p[i].waiting_time;
              avg_response += p[i].response_time;
       }

       summary->avg_turnaround = avg_turnaround / n;
       summary->avg_waiting = avg_wait / n;
       summary->avg_response = avg_response / n;
       summary->context_switches = trace_get_context_switches();
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