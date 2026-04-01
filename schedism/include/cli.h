#ifndef CLI_H
#define CLI_H

#include "process.h"

typedef struct {
    char algorithm[16];
    const char *input_path;
    const char *inline_workload;
    const char *inline_processes;
    const char *mlfq_config_path;
    int quantum;
    int compare;
    int verbose;
} CliOptions;

void print_usage(const char *prog_name);
int parse_cli(int argc, char *argv[], CliOptions *options);
int load_workload_from_options(const CliOptions *options, Process **out_processes, int *out_n);

#endif
