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
	int verbose;
	void (*trace_reset_fn)(void);
	int (*trace_add_segment_fn)(int pid_index, int start, int end);
	void (*trace_set_context_switches_fn)(int count);
	int (*trace_is_quiet_fn)(void);
} SchedulerState;

typedef struct {
	void (*reset)(void);
	int (*add_segment)(int pid_index, int start, int end);
	void (*set_context_switches)(int count);
	int (*is_quiet)(void);
} SchedulerTraceOps;

void scheduler_get_trace_ops(const SchedulerState *state, SchedulerTraceOps *ops);
int scheduler_is_verbose(const SchedulerState *state);
void scheduler_mark_process_started(Process *process, int time);
void scheduler_mark_process_completed(Process *process, int finish_time);

int schedule_fcfs(SchedulerState *state);
int schedule_sjf(SchedulerState *state);
int schedule_stcf(SchedulerState *state);
int schedule_rr(SchedulerState *state, int quantum);
int schedule_mlfq(SchedulerState *state, MLFQConfig *config);

int parse_mlfq_config(const char *path, MLFQConfig *config);
void fill_default_mlfq_config(MLFQConfig *config);
int run_algorithm(SchedulerState *state, const char *alg, int quantum, MLFQConfig *mlfq_config);
int run_single_algorithm(SchedulerState *state, const char *alg, int quantum, MLFQConfig *mlfq_config,
						 Process *processes, int n);
int run_compare_algorithms(SchedulerState *state, int rr_quantum, MLFQConfig *mlfq_config, Process *processes,
						   int n, const char *source_label);

void print_results(Process p[], int n);

#endif
