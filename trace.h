#ifndef TRACE_H
#define TRACE_H

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

#endif