#include "headers.h"

void clearResources(int);




int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop

    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}


/* 
1) I think here we read the input files the info of processes  in a queue. 
2) We have a loop, so that every second (using getClk()) we check if there are any processes to be generated.
3) If there are, we create them and pass the info to the scheduler using some ipc method such as message queue.
4) There the scheduler uses its PCB.
5) Note: if no new process arrives we need to somehow communicate to the scheduler that it should proceed and not
expect a process. (maybe send null? or use smt other than msg q)
6) Msg q or semaphores would also synchronize process generator and scheduler. 
7) Then eventually when process generator has no more processes to generate it tells the scheduler it's waiting for it to be done.
8) The scheduler will then be in a certain mode. Where after all processes are done it will tell the process generator that it's done
so that process generator can print info and exit.
*/
