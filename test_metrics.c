#include <math.h>
#include <stdio.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"

static int almost_equal(double a, double b)
{
    const double eps = 1e-9;
    return fabs(a - b) < eps;
}

static int test_empty_input(void)
{
    MetricsSummary summary;
    memset(&summary, 0xAB, sizeof(summary));

    compute_metrics_summary(NULL, 0, &summary);

    if (!almost_equal(summary.avg_turnaround, 0.0) ||
        !almost_equal(summary.avg_waiting, 0.0) ||
        !almost_equal(summary.avg_response, 0.0))
    {
        fprintf(stderr, "test_empty_input failed\n");
        return 0;
    }

    return 1;
}

static int test_sentinel_fallback(void)
{
    Process p[2];
    memset(p, 0, sizeof(p));

    strcpy(p[0].pid, "A");
    p[0].arrival_time = 0;
    p[0].burst_time = 5;
    p[0].start_time = 0;
    p[0].finish_time = 5;
    p[0].turnaround_time = 5;
    p[0].waiting_time = 0;
    p[0].response_time = 0;

    strcpy(p[1].pid, "B");
    p[1].arrival_time = 1;
    p[1].burst_time = 3;
    p[1].start_time = -1;
    p[1].finish_time = -1;
    p[1].turnaround_time = -1;
    p[1].waiting_time = -1;
    p[1].response_time = -1;

    MetricsSummary summary;
    compute_metrics_summary(p, 2, &summary);

    if (!almost_equal(summary.avg_turnaround, 2.5) ||
        !almost_equal(summary.avg_waiting, 0.0) ||
        !almost_equal(summary.avg_response, 0.0))
    {
        fprintf(stderr, "test_sentinel_fallback failed\n");
        return 0;
    }

    return 1;
}

static int test_derived_from_times(void)
{
    Process p[1];
    memset(p, 0, sizeof(p));

    strcpy(p[0].pid, "C");
    p[0].arrival_time = 2;
    p[0].burst_time = 3;
    p[0].start_time = 5;
    p[0].finish_time = 10;
    p[0].turnaround_time = -1;
    p[0].waiting_time = -1;
    p[0].response_time = -1;

    MetricsSummary summary;
    compute_metrics_summary(p, 1, &summary);

    if (!almost_equal(summary.avg_turnaround, 8.0) ||
        !almost_equal(summary.avg_waiting, 5.0) ||
        !almost_equal(summary.avg_response, 3.0))
    {
        fprintf(stderr, "test_derived_from_times failed\n");
        return 0;
    }

    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 3;

    passed += test_empty_input();
    passed += test_sentinel_fallback();
    passed += test_derived_from_times();

    printf("Metrics tests passed: %d/%d\n", passed, total);
    return passed == total ? 0 : 1;
}
