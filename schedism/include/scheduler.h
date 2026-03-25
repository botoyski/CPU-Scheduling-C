#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "metrics.h"

#define MLFQ_MAX_LEVELS 8

typedef struct {
	int levels;
	int quantum[MLFQ_MAX_LEVELS];
	int allotment[MLFQ_MAX_LEVELS];
	int boost_period;
} MLFQConfig;

typedef struct {
	int level;
	int time_quantum;
	int allotment;
	Process *queue;
	int size;
} MLFQQueue;

typedef struct {
	MLFQQueue *queues;
	int num_queues;
	int boost_period;
	int last_boost;
} MLFQScheduler;

typedef struct {
	Process *processes;
	int num_processes;
	int current_time;
	void (*trace_reset_fn)(void);
	int (*trace_add_segment_fn)(int pid_index, int start, int end);
	void (*trace_set_context_switches_fn)(int count);
	int (*trace_is_quiet_fn)(void);
} SchedulerState;

int schedule_fcfs(SchedulerState *state);
int schedule_sjf(SchedulerState *state);
int schedule_stcf(SchedulerState *state);
int schedule_rr(SchedulerState *state, int quantum);
int schedule_mlfq(SchedulerState *state, MLFQConfig *config);

void print_results(Process p[], int n);

#endif
