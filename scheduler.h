#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void schedule_fcfs(Process p[], int n);
void schedule_sjf(Process p[], int n);
void schedule_stcf(Process p[], int n);

#endif