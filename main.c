#include <stdio.h>
#include <string.h>

#include "process.h"
#include "scheduler.h"

void print_results(Process p[], int n);

int main()
{
    int n = 4;

    Process processes[] =
    {
        {1,0,8,8,0,0,0,0,0,0},
        {2,1,4,4,0,0,0,0,0,0},
        {3,2,9,9,0,0,0,0,0,0},
        {4,3,5,5,0,0,0,0,0,0}
    };

    printf("\nRunning FCFS\n");
    schedule_fcfs(processes,n);
    print_results(processes,n);

    printf("\nRunning SJF\n");
    schedule_sjf(processes,n);
    print_results(processes,n);

    printf("\nRunning STCF\n");
    schedule_stcf(processes,n);
    print_results(processes,n);

    return 0;
}