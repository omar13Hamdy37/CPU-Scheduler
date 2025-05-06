
#ifndef SCHEDULER_HELPER_H
#define SCHEDULER_HELPER_H
#include "../models/process_info.h"
#include <stdio.h>       // for printf, perror, sprintf
#include <stdlib.h>      // for exit, EXIT_FAILURE
#include <unistd.h>      // for fork, execl, getpid, sleep
#include <sys/types.h>   // for pid_t, waitpid
#include <sys/wait.h>    // for waitpid
#include <signal.h>      // for kill, SIGSTOP, SIGCONT, SIGKILL
#include <sys/ipc.h>     // for IPC_PRIVATE
#include <sys/shm.h>     // for shmget




void runProcess(ProcessInfo* process);
void pauseProcess(ProcessInfo * process);
void resumeProcess(ProcessInfo * process);
void stopProcess(ProcessInfo * process);
int CreateProcessInfoSHM_ID();

#endif