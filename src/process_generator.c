#include "headers.h"
#include "PGS_MsgQ_Utilities.h"
#include "queue.h"
#include "stdio.h"
#include "loadFile.h"

Queue *processQueue; // Queue of All Processes loaded from inputFile

// Prototype of functions
void clearResources(int);
int AllocateProcess(int time, ProcessInfo **process);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    processQueue = createQueue(); // Queue Creation
    loadInputFile("processes.txt", processQueue);
    // Todo: add parameters if needed
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    int choice;

    do
    {
        printf("Choose a Scheduling Algorithm:\n");
        printf("1. Non-preemptive Highest Priority First (HPF)\n");
        printf("2. Shortest Remaining Time Next (SRTN)\n");
        printf("3. Round Robin (RR)\n");
        printf("Enter choice (1-3): ");
        scanf("%d", &choice);

        if (choice < 1 || choice > 3)
        {
            printf("Invalid choice! Please select a valid option.\n\n");
        }

    } while (choice < 1 || choice > 3);

    // 3. Initiate and create the scheduler and clock processes.
    int msqid = createPGSchedulerMsgQueue();
    if (msqid == -1)
    {
        printf("Error creating message queue");
        exit(1);
    }
    pid_t clk_pid, scheduler_pid;

    // First fork: to start the clock process
    clk_pid = fork();
    if (clk_pid == -1)
    {
        perror("Failed to fork for clock");
        exit(1);
    }
    else if (clk_pid == 0)
    {

        execl("./bin/clk.out", "clk.out", NULL);
        perror("Failed to exec clk.out");
        exit(1);
    }

    // Second fork: to start the scheduler process
    scheduler_pid = fork();
    if (scheduler_pid == -1)
    {
        perror("Failed to fork for scheduler");
        exit(1);
    }
    else if (scheduler_pid == 0)
    {
        // Choice should be converted to string and passed as an argument
        char choice_str[10];
        sprintf(choice_str, "%d", choice);
        execl("./bin/scheduler.out", "scheduler.out", choice_str, NULL);
        perror("Failed to exec scheduler.out");
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    int timestep;
    // TODO Generation Main Loop
    // while there are still processes to arrive
    while (!isEmpty(processQueue))
    { // To Be revised later (While or do while) loop

        timestep = getClk();

        ProcessInfo *process = NULL;
        // dq all processes of current time step
        while (AllocateProcess(timestep, &process))
        {
            // send to scheduler
            SendToScheduler(*process, msqid);
        }
    }
    // wait for processes to be done
    wait(NULL);

    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

int AllocateProcess(int time, ProcessInfo **process)
{
    ProcessInfo *nextProcess = (ProcessInfo *)peek(processQueue);
    if (nextProcess == NULL)
        return 0;
    if (nextProcess->arrivalTime == time)
    {
        *process = (ProcessInfo *)dequeue(processQueue);
        return 1;
    }

    return 0;
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    destroyQueue(processQueue);
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
