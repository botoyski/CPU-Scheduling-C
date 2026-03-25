#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "gantt.h"

static ExecSegment *g_segments = NULL;
static int g_count = 0;
static int g_capacity = 0;
static int g_context_switches = 0;
static int g_quiet = 0;
/* Cursor optimizes sequential trace_pid_at_time calls (common in Gantt rendering). */
static int g_lookup_cursor = 0;

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
    g_lookup_cursor = 0;
}

void trace_free(void)
{
    free(g_segments);
    g_segments = NULL;
    g_capacity = 0;
    g_count = 0;
    g_context_switches = 0;
    g_lookup_cursor = 0;
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
    if (g_count == 0)
        return -1;

    /* Fast path for monotonically increasing time queries. */
    if (g_lookup_cursor >= 0 && g_lookup_cursor < g_count)
    {
        while (g_lookup_cursor < g_count && time >= g_segments[g_lookup_cursor].end)
            g_lookup_cursor++;

        if (g_lookup_cursor < g_count && g_segments[g_lookup_cursor].start <= time && time < g_segments[g_lookup_cursor].end)
            return g_segments[g_lookup_cursor].pid_index;

        while (g_lookup_cursor > 0 && time < g_segments[g_lookup_cursor].start)
            g_lookup_cursor--;

        if (g_segments[g_lookup_cursor].start <= time && time < g_segments[g_lookup_cursor].end)
            return g_segments[g_lookup_cursor].pid_index;
    }

    /* Fallback for random-access queries. */
    int lo = 0;
    int hi = g_count - 1;
    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        if (time < g_segments[mid].start)
        {
            hi = mid - 1;
        }
        else if (time >= g_segments[mid].end)
        {
            lo = mid + 1;
        }
        else
        {
            g_lookup_cursor = mid;
            return g_segments[mid].pid_index;
        }
    }

    if (lo > 0)
        g_lookup_cursor = lo - 1;
    else
        g_lookup_cursor = 0;

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

void print_gantt(Process p[], int n)
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
