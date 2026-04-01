// main.c is the entry point of the CPU scheduling simulator. It now delegates CLI parsing and workload loading to cli.c.

#include <stdlib.h>

#include "cli.h"
#include "gantt.h"
#include "scheduler.h"

int main(int argc, char *argv[])
{
    CliOptions options;
    if (!parse_cli(argc, argv, &options))
        return 1;

    Process *processes = NULL;
    int n = 0;
    if (!load_workload_from_options(&options, &processes, &n))
        return 1;

    SchedulerState state;
    state.processes = processes;
    state.num_processes = n;
    state.current_time = 0;
    state.verbose = options.verbose;
    state.trace_reset_fn = trace_reset;
    state.trace_add_segment_fn = trace_add_segment;
    state.trace_set_context_switches_fn = trace_set_context_switches;
    state.trace_is_quiet_fn = trace_is_quiet;

    MLFQConfig mlfq_config;
    if (options.mlfq_config_path != NULL)
    {
        if (!parse_mlfq_config(options.mlfq_config_path, &mlfq_config))
        {
            trace_free();
            free(processes);
            return 1;
        }
    }
    else
    {
        fill_default_mlfq_config(&mlfq_config);
    }

    if (options.compare)
    {
        const char *source = options.input_path != NULL ? options.input_path : "<inline-workload>";
        if (!run_compare_algorithms(&state, options.quantum, &mlfq_config, processes, n, source))
        {
            free(processes);
            return 1;
        }
    }
    else
    {
        if (!run_single_algorithm(&state, options.algorithm, options.quantum, &mlfq_config, processes, n))
        {
            trace_free();
            free(processes);
            return 1;
        }
    }

    free(processes);
    return 0;
}
