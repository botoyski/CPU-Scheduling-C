#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

#define MLFQ_MAX_LEVELS 8

typedef struct {
	double avg_turnaround;
	double avg_waiting;
	double avg_response;
	int context_switches;
} MetricsSummary;

typedef struct {
	int levels;
	int quantum[MLFQ_MAX_LEVELS];
	int allotment[MLFQ_MAX_LEVELS];
	int boost_period;
} MLFQConfig;

void schedule_fcfs(Process p[], int n);
void schedule_sjf(Process p[], int n);
void schedule_stcf(Process p[], int n);
void schedule_rr(Process p[], int n, int quantum);
void schedule_mlfq(Process p[], int n, const MLFQConfig *config);

void print_results(Process p[], int n);
void compute_metrics_summary(Process p[], int n, MetricsSummary *summary);

#endif