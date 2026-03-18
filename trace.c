#include <stdlib.h>

#include "trace.h"

static ExecSegment *g_segments = NULL;
static int g_count = 0;
static int g_capacity = 0;
static int g_context_switches = 0;
static int g_quiet = 0;

static int ensure_capacity(int needed)
{
    if (needed <= g_capacity)
        return 1;

    int new_capacity = g_capacity == 0 ? 32 : g_capacity * 2;
    while (new_capacity < needed)
        new_capacity *= 2;

    ExecSegment *grown = (ExecSegment *)realloc(g_segments, (size_t)new_capacity * sizeof(ExecSegment));
    if (grown == NULL)
        return 0;

    g_segments = grown;
    g_capacity = new_capacity;
    return 1;
}

void trace_reset(void)
{
    g_count = 0;
    g_context_switches = 0;
}

int trace_add_segment(int pid_index, int start, int end)
{
    if (start >= end)
        return 1;

    if (g_count > 0)
    {
        ExecSegment *last = &g_segments[g_count - 1];
        if (last->pid_index == pid_index && last->end == start)
        {
            last->end = end;
            return 1;
        }
    }

    if (!ensure_capacity(g_count + 1))
        return 0;

    g_segments[g_count].pid_index = pid_index;
    g_segments[g_count].start = start;
    g_segments[g_count].end = end;
    g_count++;
    return 1;
}

int trace_get_segment_count(void)
{
    return g_count;
}

const ExecSegment *trace_get_segments(void)
{
    return g_segments;
}

int trace_total_time(void)
{
    if (g_count == 0)
        return 0;
    return g_segments[g_count - 1].end;
}

int trace_pid_at_time(int time)
{
    for (int i = 0; i < g_count; i++)
    {
        if (g_segments[i].start <= time && time < g_segments[i].end)
            return g_segments[i].pid_index;
    }
    return -1;
}

void trace_set_context_switches(int count)
{
    g_context_switches = count;
}

int trace_get_context_switches(void)
{
    return g_context_switches;
}

void trace_set_quiet(int quiet)
{
    g_quiet = quiet ? 1 : 0;
}

int trace_is_quiet(void)
{
    return g_quiet;
}