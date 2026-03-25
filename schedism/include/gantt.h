#ifndef GANTT_H
#define GANTT_H

#include "process.h"

typedef struct {
	int pid_index;
	int start;
	int end;
} ExecSegment;

/*
Trace state is module-global and reused between runs.
Call trace_reset() before a new run and trace_free() when completely done.
*/
void trace_reset(void);
void trace_free(void);
int trace_add_segment(int pid_index, int start, int end);
int trace_get_segment_count(void);
const ExecSegment *trace_get_segments(void);
int trace_total_time(void);
/* Optimized for sequential time queries; supports random access as well. */
int trace_pid_at_time(int time);

void trace_set_context_switches(int count);
int trace_get_context_switches(void);

void trace_set_quiet(int quiet);
int trace_is_quiet(void);

/**
 * print_gantt - Print ASCII Gantt chart visualization of process execution
 * @p: Array of processes
 * @n: Number of processes
 *
 * Displays the execution timeline of all processes with two formats:
 * - Full ASCII Gantt chart (1 char = 1 time unit)
 * - Scaled Gantt chart (1 char = 10 time units) if total time > 80
 *
 * Uses trace information to determine which process was running at each time unit.
 */
void print_gantt(Process p[], int n);

#endif
