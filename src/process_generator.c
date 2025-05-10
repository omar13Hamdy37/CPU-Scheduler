#include "headers.h"
#include "PGS_MsgQ_Utilities.h"
#include "stdio.h"
#include "loadFile.h"

int msqid = -1;

Queue *processQueue; // Queue of All Processes loaded from inputFile

// rename sigusr1 to like tell scheduler no more remaining processes
#define PgSentAllProcesses SIGUSR1

// Prototype of functions
void clearResources(int);
int AllocateProcess(int time, ProcessInfo **process);

int main(int argc, char *argv[])
{
    // signal handler
    signal(SIGINT, clearResources);
    // process queue to store input file
    processQueue = createQueue();
    loadInputFile("processes.txt", processQueue);
    int count = getQueueCount(processQueue);

    // Pick a scheduling algorithm
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

    // If chosen algorithm is RR then ask user for quantum
    int quantum;
    if (choice == 3)
    {
        printf("Enter quantum value: ");
        scanf("%d", &quantum);
    }

    // creating the msg queue to communicate with scheduler when a process arrives
    msqid = createPGSchedulerMsgQueue();
    if (msqid == -1)
    {
        printf("Error creating message queue");
        exit(1);
    }
    pid_t clk_pid, scheduler_pid;

    // start clock process
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
    // start scheduler process
    scheduler_pid = fork();
    if (scheduler_pid == -1)
    {
        perror("Failed to fork for scheduler");
        exit(1);
    }
    else if (scheduler_pid == 0)
    {
        // Choice, size and quantum should be converted to string and passed as an argument
        char choice_str[10];
        char count_str[10];
        char quantum_str[10];
        sprintf(choice_str, "%d", choice);
        sprintf(count_str, "%d", count);
        sprintf(quantum_str, "%d", quantum);
        execl("./bin/scheduler.out", "scheduler.out", choice_str, count_str, quantum_str, NULL);
        perror("Failed to exec scheduler.out");
    }
    // initalize clock
    initClk();
    int timestep;


 
    // while there are still processes to arrive
    while (!isEmpty(processQueue))
    {

        timestep = getClk();

        ProcessInfo *process = NULL;
        // allocate all processes of current timestep
        while (AllocateProcess(timestep, &process))
        {
            // send to scheduler
            SendToScheduler(*process, msqid);

        }
    }
    // tell scheduler u sent all processes (useful for deciding when algo is done)

    kill(scheduler_pid, PgSentAllProcesses);

    // wait for scheduler to be done
    waitpid(scheduler_pid, NULL, 0);

    clearResources(0);
}
// function to check if it's the time of arrival of this process
// if yes dequeue
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
// clear resources (called and used as a signal handler)

void clearResources(int signum)
{

    ClearMsgQ(msqid);
    destroyQueue(processQueue);
    destroyClk(true, 1);
}
