

#ifndef SCHEDULER_ALGO_H
#define SCHEDULER_ALGO_H

#include "../models/process_info.h"
#include "Scheduler_log.h"
#include "stdio.h"
#include <sys/types.h>
#include "queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include "../include/PGS_MsgQ_Utilities.h"
#include <sys/ipc.h>   // For IPC_PRIVATE, etc.


#include <signal.h>


#include <limits.h>

void runHPF(int msqid);
void runSRTN(int msqid);
void runRR();
ProcessInfo* dequeue_highest_priority(Queue* q); //used for HPF
ProcessInfo* dequeueShortestJob(Queue* q); //used for SRTN
//Helper Functions
void runProcess(ProcessInfo* process);
void pauseProcess(ProcessInfo * process);
void resumeProcess(ProcessInfo * process);
void stopProcess(ProcessInfo * process);

#endif