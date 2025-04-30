#include "../models/process_info.h"
#include "stdio.h"

//this func is called in the begginning of scheduler.c to initialize the output file
void initSchedulerLog();
// Call this function eachtime a process changes its state (RUNNING, STOPPED, etc...) 
void logProcess(ProcessInfo* process, int timestep, ProcessState newState);