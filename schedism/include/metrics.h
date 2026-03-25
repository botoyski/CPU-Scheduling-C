#ifndef METRICS_H
#define METRICS_H

#include "process.h"

typedef struct {
	double avg_turnaround;
	double avg_waiting;
	double avg_response;
	int context_switches;
} MetricsSummary;

void compute_metrics_summary(Process p[], int n, MetricsSummary *summary);

#endif
