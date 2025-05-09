#ifndef MEMORYLOG_H
#define MEMORYLOG_H

#include "../models/process_info.h"
#include "stdio.h"

//initialize the output file
void initMemoryLog();
// Logs memory info once a process is allocated or deallocated 
void processMemoryLog(ProcessInfo* process, int timestep, int action, int size); // action = 1 for allocation & 0 for deallocation

#endif