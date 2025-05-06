#include "../models/process_info.h"
#include "Scheduler_log.h"
#include "stdio.h"
#include <sys/types.h>

#include <limits.h>

#ifndef SCHEDULER_ALGO_H
#define SCHEDULER_ALGO_H
void runHPF(int msqid);
void runSRTN(int msqid);
void runRR();
ProcessInfo* dequeue_highest_priority(Queue* q); //used for HPF
ProcessInfo* dequeueShortestJob(Queue* q); //used for SRTN
//Helper Functions
void runProcess(ProcessInfo* process);
void pauseProcess(pid_t pid);
void resumeProcess(pid_t pid);
void stopProcess(pid_t pid);

#endif