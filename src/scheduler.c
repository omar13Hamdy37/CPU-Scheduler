
#include "../models/scheduling_types.h"
#include "headers.h"
#include "PGS_MsgQ_Utilities.h"
#include "Scheduler_log.h"
#include <stdlib.h> // for atoi
#include "queue.h"
#include <limits.h> // for INT_MIN
#include <math.h>   // for pow
#include "scheduler_algo.h"
#include "scheduler_stats.h"
#include "priQueue.h"
#include "scheduler_helper.h"

#define PgSentAllProcesses SIGUSR1
// bool saying pg is done sending processes
bool PgDone = false;
void clearScheduler(int sig);
// if done sig is sent and we change that bool
Queue *finishQueue;
Queue *readyQueue;
PriQueue *priReadyQueue;

void handle_Pg_Done(int sig)
{
    if (sig == PgSentAllProcesses)
    {
        PgDone = true;
        printf("SIGNAL RECEIVED\n");
    }
}

void runRoundRobin(int quantum);
void runSRTN_new(int msqid);
void entireRR(int quantum);
// void runHPF();

int totalNumProcesses;
int msqid; // id of message queue we use to communicate with PG

int main(int argc, char *argv[])
{
    signal(SIGINT, clearScheduler);

    signal(PgSentAllProcesses, handle_Pg_Done);
    // initalize clock
    initClk();
    // get scheduling type from arguments ( we convert argument to int as it was string then to enum)
    enum scheduling_type algorithm = (enum scheduling_type)atoi(argv[1]);
    totalNumProcesses = atoi(argv[2]);
    // Get msqid
    msqid = createPGSchedulerMsgQueue();
    if (msqid == -1)
    {
        printf("Error getting msqid\n");
        exit(1);
    }

    // For RR
    int quantum;
    // Print to check algorithm choice correct
    switch (algorithm)
    {
    case HPF:
        printf("Scheduling Algorithm is: Non-preemptive Highest Priority First (HPF)\n");
        // runHPF(msqid);
        break;
    case SRTN:
        printf("Scheduling Algorithm is: Shortest Remaining Time Next (SRTN)\n");

        runSRTN_new(msqid);

        break;
    case RR:
        printf("Scheduling Algorithm is: Round Robin (RR)\n");
        printf("Enter quantum value: ");
        scanf("%d", &quantum);
        entireRR(quantum);
        break;
    default:
        printf("Invalid scheduling algorithm selected.\n");
        exit(1);
    }
    clearScheduler(-1);
    // last thing as it clears finish queue
    schedulerPerf(finishQueue);
    destroyQueue(finishQueue);
}
/*
1) The scheduler needs to have these data ready at the end. So here is what data you need to keep track of:
    a) CPU UTILIZATION -> time cpu used, total time
    b) average weighted turnaround time -> when process is done it calculates the turnaround time and adds it to the total
    c) average waiting time -> when process is done it calculates the waiting time and adds it to the total
    d) Standard deviation for average weighted turnaround time -> u have the info to calculate it

2) So when a process is done it sends a message to the scheduler with the data it needs to calculate the above.
3) How does the process send that data to the scheduler? Probably using a message queue or signal.
*/

void runRoundRobin(int quantum)
{
    // queue of ready processes info

    int size = getQueueCount(readyQueue);
    for (int i = 0; i < size; i++)
    {
        ProcessInfo *proc = (ProcessInfo *)dequeue(readyQueue);

        if (proc->state == WAITING)
        {
            proc->startTime = getClk();
            proc->waitingTime = proc->startTime - proc->arrivalTime;

            logProcess(proc, getClk(), STARTED);
        }
        else
        {

            logProcess(proc, getClk(), RESUMED);
        }
        // min of remaining and quantum
        int timeToRun = (proc->remainingTime < quantum) ? proc->remainingTime : quantum;

        // Simulate running
        int start = getClk();
        sleep(timeToRun); // Sleep for the duration of timeToRun

        proc->remainingTime -= timeToRun;

        if (proc->remainingTime <= 0)
        {
            proc->endTime = getClk();
            logProcess(proc, getClk(), FINISHED);
            enqueue(finishQueue, proc);
        }
        else
        { // still needs time for processing

            logProcess(proc, getClk(), STOPPED);
            enqueue(readyQueue, proc);
        }
    }
}

// Keep these functions
void runSRTN_new(int msqid)
{
    int count = 0;
    // TODO: CHECK THESE INIT ARE OK
    // initalize clock

    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    // queue for ready processes
    // pri queue and srt is the priority thing
    priReadyQueue = createPriQueue();

    finishQueue = createQueue();

    // TODO: FIX WHILE LOOP CONDITION
    // 3ayz while pg not done or queue not empty
    ProcessInfo *currentProcess = NULL;
    ProcessInfo *incomingProcess = NULL;
    int *pri = malloc(sizeof(int));
    // TODO: EDGECASE BA3AT AND I DIDNT RECEIVE AND LIST EMPTY
    while (count != totalNumProcesses)
    {

        // // Blocks and waits till it receives a process

        // BlockingReceiveFromPG(&msgBuffer, msqid);
        // // Unpack to get process info
        // ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);
        // // create process info in shared memory to share with actual process
        // int shmid = CreateProcessInfoSHM_ID();

        // // attch to scheduler memory
        // ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

        // // copy info to shared memory
        // *process = unpackedProcess;
        // process->shmid = shmid;

        // // TODO: REMOVE LATER (FOR NOW CHECK THAT PROCESS COMES AT CORRECT TIME)

        // printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
        //        process->pid, getClk(), process->arrivalTime);
        // // negative remaining time so srt has highest prioity
        // enqueuePri(priReadyQueue, process, -(process->remainingTime));

        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);

            int shmid = CreateProcessInfoSHM_ID();

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

            *process = unpackedProcess;
            process->shmid = shmid;

            //         // TODO: REMOVE LATER (FOR NOW CHECK THAT PROCESS COMES AT CORRECT TIME)

            printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
                   process->pid, getClk(), process->arrivalTime);
            // negative remaining time so srt has highest prioity
            enqueuePri(priReadyQueue, process, -(process->remainingTime));
        }
        //--------------------------------------------

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
            {

                runProcess(currentProcess);
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                currentProcess->startTime = getClk();
                logProcess(currentProcess, getClk(), STARTED);
            }
        }

        // if you are an incoming process and less rt you can run

        else if (peekPri(priReadyQueue, (void **)&incomingProcess, pri) != 0)
        {

            if (incomingProcess->remainingTime < currentProcess->remainingTime)
            {
                pauseProcess(currentProcess);
                enqueuePri(priReadyQueue, currentProcess, -(currentProcess->remainingTime));
                currentProcess->lastStopTime = getClk();
                logProcess(currentProcess, getClk(), STOPPED);

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {
                    runProcess(currentProcess);
                    ProcessState state;
                    if (currentProcess->state == STOPPED)
                    {
                        currentProcess->waitingTime += getClk() - currentProcess->lastStopTime;
                        state = RESUMED;
                    }
                    else
                    {
                        state = STARTED;
                        currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                        currentProcess->startTime = getClk();
                    }

                    logProcess(currentProcess, getClk(), state);
                }
            }
        }

        // if process done
        if (currentProcess != NULL)
        {

            if (currentProcess->remainingTime <= 0)
            {
                // end
                if (currentProcess->state != FINISHED)
                {
                    count++;

                    currentProcess->endTime = getClk();
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess, getClk(), FINISHED);
                }

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {

                    runProcess(currentProcess);
                    ProcessState state;
                    if (currentProcess->state == STOPPED)
                    {
                        currentProcess->waitingTime += getClk() - currentProcess->lastStopTime;
                        state = RESUMED;
                    }
                    else
                    {
                        currentProcess->startTime = getClk();
                        state = STARTED;
                        currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                    }

                    logProcess(currentProcess, getClk(), state);
                }
            }
        }
    }
    printPointersQueue(finishQueue, printProcessInfoPtr);


}
void entireRR(int quantum)
{
    // queue of ready processes info

    // Create queue of ready processes
    readyQueue = createQueue();
    // Create queue of finished process
    finishQueue = createQueue();

    // Initiate the Scheduler.log file
    initSchedulerLog();

    // message buffer and send type to 1
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    int algoPid = fork();

    while (getClk() < 50)
    {
        // Recieve message
        int msgStatus = ReceiveFromPG(&msgBuffer, msqid);

        if (msgStatus == -1)
        {
            // If readyQueue has processes, keep running algorithm
            if (!isEmpty(readyQueue))
            {
                runRoundRobin(quantum);
            }
        }
        else
        {

            // Unpack to get process info
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);
            // create process info (dynamic as to avoid rewrite)
            ProcessInfo *processCopy = (ProcessInfo *)malloc(sizeof(ProcessInfo));
            *processCopy = unpackedProcess;

            // enqueue that process
            enqueue(readyQueue, processCopy);

            // print to check ready queue is correct
            printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
                   processCopy->pid, getClk(), processCopy->arrivalTime);

            if (!isEmpty(readyQueue))
            {
                runRoundRobin(quantum);
            }
        }
    }

    // TODO implement the scheduler :)
    // ana edetak aho the type of scheduling algo check models hatal2y enum
    // upon termination release the clock resources.

}

// Function to clear all shared memory of all processes
void clearScheduler(int sig)
{

    destroyClk(false);
    printf("SCHEDULER CLEARED\n");
    if (finishQueue != NULL)
    {

        QueueNode *current = finishQueue->front;

        // loop
        while (current != NULL)
        {

            ProcessInfo *proc = (ProcessInfo *)current->data;
            if (proc->shmid != -1)
            {
                // get shmid (must get it men hena before dettaching)
                int shmid = proc->shmid;
                if (shmdt(proc) == -1)
                {
                    perror("Failed to detach shared memory");
                }

                // delete shm from kernel
                if (shmctl(shmid, IPC_RMID, NULL) == -1)
                {
                    perror("Failed to remove shared memory");
                }
            }

            current = current->next;
        }
    }
    if (priReadyQueue != NULL)
    {

        PriNode *current = priReadyQueue->head;

        // loop
        while (current != NULL)
        {

            ProcessInfo *proc = (ProcessInfo *)current->item;
            if (proc->shmid != -1)
            {
                // get shmid (must get it men hena before dettaching)
                int shmid = proc->shmid;
                if (shmdt(proc) == -1)
                {
                    perror("Failed to detach shared memory");
                }

                // delete shm from kernel
                if (shmctl(shmid, IPC_RMID, NULL) == -1)
                {
                    perror("Failed to remove shared memory");
                }
            }

            current = current->next;
        }
    }
    if (readyQueue != NULL)
    {

        QueueNode *current = readyQueue->front;

        // loop
        while (current != NULL)
        {

            ProcessInfo *proc = (ProcessInfo *)current->data;
            if (proc->shmid != -1)
            {
                // get shmid (must get it men hena before dettaching)
                int shmid = proc->shmid;
                if (shmdt(proc) == -1)
                {
                    perror("Failed to detach shared memory");
                }

                // delete shm from kernel
                if (shmctl(shmid, IPC_RMID, NULL) == -1)
                {
                    perror("Failed to remove shared memory");
                }
            }

            current = current->next;
        }
    }
    destroyPriQueue(priReadyQueue);
    destroyQueue(readyQueue);
    if (sig == SIGINT)
    {
        printf("aywa sigint\n");
        destroyQueue(finishQueue);
    }
    

}
