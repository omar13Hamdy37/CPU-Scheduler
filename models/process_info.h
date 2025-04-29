#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H
// process struct
typedef struct 
{
    int pid;         // useful for scheduler to control process.
    int arrivalTime; // useful for process_generator, scheduler, stats.
    int startTime;   // useful for stats
    int runTime;     // useful for scheduler, process itself, stats
    int endTime;     // useful for stats
    int waitingTime; // useful for stats
    int priority;    // useful for scheduling algorithms

    int remainingTime;          // useful for process itself, scheduler
    int turnaroundTime;         // useful for stats
    int weightedTurnaroundTime; // useful for stats
} ProcessInfo;
#endif