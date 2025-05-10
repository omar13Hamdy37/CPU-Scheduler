#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H
#include <stdio.h>

// process states
typedef enum {
    WAITING,
    STARTED,
    STOPPED,
    RESUMED,
    FINISHED
} ProcessState;

// process struct
typedef struct 
{
    int actualPid; //(ONCE PROCESS RUNS)
    int pid;         // useful for scheduler to control process.    (FROM INPUT FILE)
    int arrivalTime; // useful for process_generator, scheduler, stats.
    int startTime;   // useful for stats
    int runTime;     // useful for scheduler, process itself, stats
    int endTime;     // useful for stats
    int waitingTime; // useful for stats
    int priority;    // useful for scheduling algorithms
    int lastStopTime; // used to calculate wait time while processing
    int memsize;     // for memory size
    int address;     // pointer to start of address space in memory

    int remainingTime;          // useful for process itself, scheduler
    int turnaroundTime;         // useful for stats
    int weightedTurnaroundTime; // useful for stats
    int shmid;
    int schedulerPid;
    int semid;
    ProcessState state; // useful for tracking process state
} ProcessInfo;


// STATIC INLINE: STATIC MAKES THE FUNCTION PRIVATE FOR EACH .C FILE THAT INCLUDES IT
// BOS THIS PREVENTS THE MULTIPLE DEF ERROR


// function to print process info pointer bcz of queues(for testing remove later)
// void * 3ashan t3rf tedkhol the print pri queue function
// then type cast to procesinfo pointer 3ady
static inline void printProcessInfoPtr( void* pe) {
    if (pe == NULL) {
        printf("Process: EMPTY\n");
        return;
    }
    ProcessInfo * p = (ProcessInfo*) pe;

    printf("Process: { PID: %d, Arrival: %d, Start: %d, Run: %d, End: %d, Waiting: %d, Priority: %d, Remaining: %d, Turnaround: %d, WTA: %d, State: %d }\n",
           p->pid, p->arrivalTime, p->startTime, p->runTime, p->endTime, p->waitingTime,
           p->priority, p->remainingTime, p->turnaroundTime, p->weightedTurnaroundTime, p->state);
}

#endif