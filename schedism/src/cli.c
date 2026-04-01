#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "utils.h"

static int load_processes_inline_generic(const char *spec, const char *item_delim,
                                         const char *field_delim, const char *option_name,
                                         Process **out_processes, int *out_n)
{
    if (spec == NULL || *spec == '\0')
    {
        fprintf(stderr, "Error: --%s must not be empty.\n", option_name);
        return 0;
    }

    char *buf = (char *)malloc(strlen(spec) + 1);
    if (buf == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 0;
    }
    strcpy(buf, spec);

    int capacity = 16;
    int count = 0;
    Process *processes = (Process *)malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed.\n");
        free(buf);
        return 0;
    }

    char *saveptr = NULL;
    char *entry = strtok_r(buf, item_delim, &saveptr);
    while (entry != NULL)
    {
        char *item = trim_spaces(entry);
        if (*item != '\0')
        {
            char *c1 = strchr(item, *field_delim);
            char *c2 = c1 ? strchr(c1 + 1, *field_delim) : NULL;
            char *c3 = c2 ? strchr(c2 + 1, *field_delim) : NULL;

            if (c1 == NULL || c2 == NULL || c3 != NULL)
            {
                fprintf(stderr, "Error: invalid --%s entry '%s'. Expected PID%cARRIVAL%cBURST\n",
                        option_name, item, *field_delim, *field_delim);
                free(processes);
                free(buf);
                return 0;
            }

            *c1 = *c2 = '\0';
            char *pid = trim_spaces(item);
            int arrival_time;
            int burst_time;
            if (*pid == '\0' || !parse_int_strict(trim_spaces(c1 + 1), &arrival_time) ||
                !parse_int_strict(trim_spaces(c2 + 1), &burst_time))
            {
                fprintf(stderr, "Error: invalid --%s entry.\n", option_name);
                free(processes);
                free(buf);
                return 0;
            }

            if (count == capacity)
            {
                capacity *= 2;
                Process *resized = (Process *)realloc(processes, (size_t)capacity * sizeof(Process));
                if (resized == NULL)
                {
                    fprintf(stderr, "Error: memory allocation failed.\n");
                    free(processes);
                    free(buf);
                    return 0;
                }
                processes = resized;
            }

            strncpy(processes[count].pid, pid, sizeof(processes[count].pid) - 1);
            processes[count].pid[sizeof(processes[count].pid) - 1] = '\0';
            processes[count].arrival_time = arrival_time;
            processes[count].burst_time = burst_time;
            count++;
        }
        entry = strtok_r(NULL, item_delim, &saveptr);
    }

    free(buf);

    if (count == 0)
    {
        fprintf(stderr, "Error: no processes found in --%s.\n", option_name);
        free(processes);
        return 0;
    }

    process_reset_all(processes, count);
    *out_processes = processes;
    *out_n = count;
    return 1;
}

static int load_workload_inline(const char *spec, Process **out_processes, int *out_n)
{
    return load_processes_inline_generic(spec, ";", ",", "workload", out_processes, out_n);
}

static int load_processes_inline(const char *spec, Process **out_processes, int *out_n)
{
    return load_processes_inline_generic(spec, ",", ":", "processes", out_processes, out_n);
}

static int load_workload_file(const char *path, Process **out_processes, int *out_n)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: cannot open input file '%s'.\n", path);
        return 0;
    }

    int capacity = 16;
    int count = 0;
    Process *processes = (Process *)malloc((size_t)capacity * sizeof(Process));
    if (processes == NULL)
    {
        fclose(fp);
        fprintf(stderr, "Error: memory allocation failed.\n");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (is_blank_or_comment(line))
            continue;

        char pid[16];
        int arrival_time;
        int burst_time;
        if (sscanf(line, "%15s %d %d", pid, &arrival_time, &burst_time) != 3)
        {
            fprintf(stderr, "Error: invalid workload line: %s", line);
            free(processes);
            fclose(fp);
            return 0;
        }

        if (count == capacity)
        {
            capacity *= 2;
            Process *resized = (Process *)realloc(processes, (size_t)capacity * sizeof(Process));
            if (resized == NULL)
            {
                fprintf(stderr, "Error: memory allocation failed.\n");
                free(processes);
                fclose(fp);
                return 0;
            }
            processes = resized;
        }

        strncpy(processes[count].pid, pid, sizeof(processes[count].pid) - 1);
        processes[count].pid[sizeof(processes[count].pid) - 1] = '\0';
        processes[count].arrival_time = arrival_time;
        processes[count].burst_time = burst_time;
        count++;
    }

    fclose(fp);

    if (count == 0)
    {
        fprintf(stderr, "Error: no processes found in input file '%s'.\n", path);
        free(processes);
        return 0;
    }

    process_reset_all(processes, count);
    *out_processes = processes;
    *out_n = count;
    return 1;
}

void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --input=workload.txt [options]\n", prog_name);
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --workload=\"P1,0,5;P2,1,3\" [options]\n", prog_name);
    printf("  %s --algorithm=FCFS|SJF|STCF|RR|MLFQ --processes=\"P1:0:5,P2:1:3\" [options]\n", prog_name);
    printf("  %s --compare --input=workload.txt [options]\n", prog_name);
    printf("  %s --compare --workload=\"P1,0,5;P2,1,3\" [options]\n", prog_name);
    printf("  %s --compare --processes=\"P1:0:5,P2:1:3\" [options]\n", prog_name);
    printf("\nOptions:\n");
    printf("  --quantum=<q>           Time quantum for RR (default: 30)\n");
    printf("  --mlfq-config=<file>    MLFQ config file path\n");
    printf("  --compare               Run all algorithms on the same workload\n");
    printf("  --verbose               Show per-process scheduling logs\n");
    printf("  --workload=<spec>       Inline workload: PID,ARRIVAL,BURST;...\n");
    printf("  --processes=<spec>      Inline processes: PID:ARRIVAL:BURST,...\n");
}

int parse_cli(int argc, char *argv[], CliOptions *options)
{
    strncpy(options->algorithm, "FCFS", sizeof(options->algorithm) - 1);
    options->algorithm[sizeof(options->algorithm) - 1] = '\0';
    options->input_path = NULL;
    options->inline_workload = NULL;
    options->inline_processes = NULL;
    options->mlfq_config_path = NULL;
    options->quantum = 30;
    options->compare = 0;
    options->verbose = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "--algorithm=", 12) == 0)
        {
            const char *alg = argv[i] + 12;
            if (strlen(alg) >= sizeof(options->algorithm))
            {
                fprintf(stderr, "Error: algorithm name too long: '%s'.\n", alg);
                return 0;
            }
            strncpy(options->algorithm, alg, sizeof(options->algorithm) - 1);
            options->algorithm[sizeof(options->algorithm) - 1] = '\0';
        }
        else if (strncmp(argv[i], "--input=", 8) == 0)
        {
            options->input_path = argv[i] + 8;
        }
        else if (strncmp(argv[i], "--workload=", 11) == 0)
        {
            options->inline_workload = argv[i] + 11;
        }
        else if (strncmp(argv[i], "--processes=", 12) == 0)
        {
            options->inline_processes = argv[i] + 12;
        }
        else if (strncmp(argv[i], "--quantum=", 10) == 0)
        {
            options->quantum = atoi(argv[i] + 10);
        }
        else if (strncmp(argv[i], "--mlfq-config=", 14) == 0)
        {
            options->mlfq_config_path = argv[i] + 14;
        }
        else if (equals_ignore_case(argv[i], "--compare"))
        {
            options->compare = 1;
        }
        else if (equals_ignore_case(argv[i], "--verbose"))
        {
            options->verbose = 1;
        }
        else if (equals_ignore_case(argv[i], "--help"))
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Error: unknown argument '%s'.\n", argv[i]);
            print_usage(argv[0]);
            return 0;
        }
    }

    int sources = 0;
    if (options->input_path != NULL)
        sources++;
    if (options->inline_workload != NULL)
        sources++;
    if (options->inline_processes != NULL)
        sources++;

    if (sources == 0)
    {
        fprintf(stderr, "Error: provide one workload source: --input, --workload, or --processes.\n");
        print_usage(argv[0]);
        return 0;
    }

    if (sources > 1)
    {
        fprintf(stderr, "Error: use only one workload source: --input, --workload, or --processes.\n");
        print_usage(argv[0]);
        return 0;
    }

    if (equals_ignore_case(options->algorithm, "RR") && options->quantum <= 0)
    {
        fprintf(stderr, "Error: RR quantum must be positive.\n");
        return 0;
    }

    if (!options->compare && equals_ignore_case(options->algorithm, "MLFQ") && options->mlfq_config_path == NULL)
    {
        fprintf(stderr, "Error: --mlfq-config is required for MLFQ.\n");
        return 0;
    }

    return 1;
}

int load_workload_from_options(const CliOptions *options, Process **out_processes, int *out_n)
{
    if (options->input_path != NULL)
        return load_workload_file(options->input_path, out_processes, out_n);
    if (options->inline_processes != NULL)
        return load_processes_inline(options->inline_processes, out_processes, out_n);
    return load_workload_inline(options->inline_workload, out_processes, out_n);
}
