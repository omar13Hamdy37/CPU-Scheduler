
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
#include "buddySystem_UT.h"
#include "memoryLog.h"

// if done sig is sent and we change that bool
#define PgSentAllProcesses SIGUSR1
// bool saying pg is done sending processes
bool PgDone = false;

void clearScheduler(int sig);

// Queues that we will use
Queue *finishQueue;
Queue *readyQueue;
Queue *waitingQ; // for processes that we couldn't allocate memory for
Queue *tempQ;
PriQueue *priReadyQueue;

// Memory root
MemoryTreeNode* memoryTreeRoot;

// function to modify PgDone when pg is done
void handle_Pg_Done(int sig)
{
    if (sig == PgSentAllProcesses)
    {
        PgDone = true;
    }
}

void runSRTN_new(int msqid);
void runHPF_new(int msqid);
void runRR_new(int msqid, int quantum);

// deallocate process from memory
void deallocateProcessMemory(ProcessInfo* process);
// num processes
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

    // Initializing memory tree
    memoryTreeRoot = createMemoryTree();

    // Print to check algorithm choice correct
    switch (algorithm)
    {
    case HPF:
        printf("Scheduling Algorithm is: Non-preemptive Highest Priority First (HPF)\n");
        runHPF_new(msqid);
        break;
    case SRTN:
        printf("Scheduling Algorithm is: Shortest Remaining Time Next (SRTN)\n");
        runSRTN_new(msqid);
        break;
    case RR:
        printf("Scheduling Algorithm is: Round Robin (RR)\n");
        int quantum = atoi(argv[3]);
        runRR_new(msqid, quantum);
        break;
    default:
        printf("Invalid scheduling algorithm selected.\n");
        exit(1);
    }

    // create perf file
    schedulerPerf(finishQueue);
    // clear scheduler resources
    clearScheduler(-1);
    // destroy final queue
    destroyQueue(finishQueue);
}

void runSRTN_new(int msqid)
{
    int count = 0;

    // initalize to be able to log
    initSchedulerLog();
    initMemoryLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    // queue for ready processes

    // pri queue and srt is the priority thing
    priReadyQueue = createPriQueue();
    finishQueue = createQueue();
    waitingQ = createQueue();
    tempQ = createQueue();

    ProcessInfo *currentProcess = NULL;
    ProcessInfo *incomingProcess = NULL;
    int *pri = malloc(sizeof(int)); // (not really used)

    while (count != totalNumProcesses)
    {

        // try to allocate memory for all processes in waitingQ
        while (!isEmpty(waitingQ)) {
            ProcessInfo *process = (ProcessInfo *)dequeue(waitingQ);
            int size = getNearestPowerOfTwo(process->memsize);
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);

            // if we couldnt find best fit return process to waitingqueue
            if (currentBestFit == NULL) {
                enqueue(tempQ, process);
            }
            else {
                // get address (start byte) of process memory
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // enqueue to priorityQ
                enqueuePri(priReadyQueue, process, -(process->remainingTime));
                processMemoryLog(process, getClk(), 1, size);
            }
        }
        // return processes from temp to waitingQ
        while (!isEmpty(tempQ))
        {
            ProcessInfo *process = (ProcessInfo *)dequeue(tempQ);
            enqueue(waitingQ, process);
        }
        // keep on receiving processes from msg q till it's empty

        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            // unpack from msg q
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);
            // get shmid for this process
            int shmid = CreateProcessInfoSHM_ID();
            // attach to the scheduler

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);
            // copy proces to shared memory
            *process = unpackedProcess;
            // put shmid inside process
            process->shmid = shmid;

            // try to allocate memory to process
            int size = getNearestPowerOfTwo(process->memsize);
            // get best fit for process in the current memory tree
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);
            // if we couldn't allocate memory for process enqueue it to waiting Q
            if (currentBestFit == NULL) {
                enqueue(waitingQ, process);
            }
            else
            {
                // get address of allocated process
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // negative remaining time so srt has highest prioity
                enqueuePri(priReadyQueue, process, -(process->remainingTime));
                processMemoryLog(process, getClk(), 1, size);
            }
        }

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
            {
                // deq from ready and run that process
                runProcess(currentProcess);
                // it's waiting time is current - arrival
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                // start time is rn
                currentProcess->startTime = getClk();
                // log current state
                logProcess(currentProcess, getClk(), STARTED);
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
                    // count will increse
                    count++;
                    currentProcess->endTime = getClk();
                    // enqueue to finish queue and log
                    deallocateProcessMemory(currentProcess);
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess, getClk(), FINISHED);
                }
                // if there is a proces available run now
                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {
                    // run
                    runProcess(currentProcess);
                    ProcessState state;
                    // if process is stopped then it will be resumed
                    if (currentProcess->state == STOPPED)
                    {
                        currentProcess->waitingTime += getClk() - currentProcess->lastStopTime;
                        state = RESUMED;
                    }
                    else
                    {
                        // else it just came to it should be start
                        currentProcess->startTime = getClk();
                        state = STARTED;
                        currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                    }
                    // log with the correct state
                    logProcess(currentProcess, getClk(), state);
                }
            }
        }
        // if you are an incoming process and less rt you can run

        if (peekPri(priReadyQueue, (void **)&incomingProcess, pri) != 0)
        {

            if (incomingProcess->remainingTime < currentProcess->remainingTime)
            {
                // pause current process
                pauseProcess(currentProcess);
                // enqueue it in ready
                enqueuePri(priReadyQueue, currentProcess, -(currentProcess->remainingTime));
                // update last stop time to use in calc wait time
                currentProcess->lastStopTime = getClk();
                logProcess(currentProcess, getClk(), STOPPED);

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {
                    // dequeue next one
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
    }
}

void runHPF_new(int msqid)
{
    int count = 0;

    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    initMemoryLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    // queue for ready processes
    // pri queue and pri is the priority thing
    priReadyQueue = createPriQueue();
    finishQueue = createQueue();
    waitingQ = createQueue();
    tempQ = createQueue();

    ProcessInfo *currentProcess = NULL;
    int *pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {
        // try to allocate memory for all processes in waitingQ
        while (!isEmpty(waitingQ)) {
            ProcessInfo *process = (ProcessInfo *)dequeue(waitingQ);
            int size = getNearestPowerOfTwo(process->memsize);
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);

            // if we couldnt find best fit return process to waitingQ
            if (currentBestFit == NULL) {
                enqueue(tempQ, process);
            }
            else {
                // get address (start byte) of process memory
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // enqueue to priorityQ
                enqueuePri(priReadyQueue, process, -(process->priority));
                processMemoryLog(process, getClk(), 1, size);
            }
        }
        // return processes from tempQ to waitingQ
        while (!isEmpty(tempQ))
        {
            ProcessInfo *process = (ProcessInfo *)dequeue(tempQ);
            enqueue(waitingQ, process);
        }
        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);

            int shmid = CreateProcessInfoSHM_ID();

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

            *process = unpackedProcess;
            process->shmid = shmid;

            // try to allocate memory to process
            int size = getNearestPowerOfTwo(process->memsize);
            // get best fit for process in the current memory tree
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);
            // if we couldn't allocate memory for process enqueue it to waiting Q
            if (currentBestFit == NULL) {
                enqueue(waitingQ, process);
            }
            else
            {
                // get address of allocated process
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // negative priority so pri has highest prioity
                enqueuePri(priReadyQueue, process, -(process->priority));
                processMemoryLog(process, getClk(), 1, size);
            }

        }

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
                    deallocateProcessMemory(currentProcess);
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess, getClk(), FINISHED);
                }

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {

                    runProcess(currentProcess);
                    currentProcess->startTime = getClk();
                    currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;

                    logProcess(currentProcess, getClk(), STARTED);
                }
            }
        }
    }
}
void runRR_new(int msqid, int quantum)
{
    int count = 0;
    // initalize clock

    // initalize output files to be able to log
    initSchedulerLog();
    initMemoryLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;

    // Time at which current process started its runningTime
    int prevStartTime;
    // queue for ready processes
    // pri queue and pri is the priority thing (used for sorting processes with same arrival time by their priority)
    priReadyQueue = createPriQueue();
    // Actual readyQueue for RR
    readyQueue = createQueue();
    finishQueue = createQueue();
    waitingQ = createQueue();
    tempQ = createQueue();

    // 3ayz while pg not done or queue not empty
    ProcessInfo *currentProcess = NULL;
    ProcessInfo *temp = NULL;
    int *pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {
        // try to allocate memory for all processes in waitingQ
        while (!isEmpty(waitingQ)) {
            ProcessInfo *process = (ProcessInfo *)dequeue(waitingQ);
            int size = getNearestPowerOfTwo(process->memsize);
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);

            // if we couldnt find best fit return process to waitingQ
            if (currentBestFit == NULL) {
                enqueue(tempQ, process);
            }
            else {
                // get address (start byte) of process memory
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // enqueue to priorityQ
                enqueuePri(priReadyQueue, process, -(process->priority));
                processMemoryLog(process, getClk(), 1, size);
            }
        }
        // return processes from tempQ to waitingQ
        while (!isEmpty(tempQ))
        {
            ProcessInfo *process = (ProcessInfo *)dequeue(tempQ);
            enqueue(waitingQ, process);
        }
        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);

            int shmid = CreateProcessInfoSHM_ID();

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

            *process = unpackedProcess;
            process->shmid = shmid;

            // try to allocate memory to process
            int size = getNearestPowerOfTwo(process->memsize);
            // get best fit for process in the current memory tree
            MemoryTreeNode* currentBestFit = getBestFit(memoryTreeRoot, size);
            // if we couldn't allocate memory for process enqueue it to waiting Q
            if (currentBestFit == NULL) {
                enqueue(waitingQ, process);
            }
            else
            {
                // get address of allocated process
                int address = allocateMemory(currentBestFit, process->memsize);
                process->address = address;
                // negative priority so pri has highest prioity
                enqueuePri(priReadyQueue, process, -(process->priority));
                processMemoryLog(process, getClk(), 1, size);
            }
        }
        // dequeue the sorted process to the actual RR ready queue
        while (dequeuePri(priReadyQueue, (void **)&temp, pri) != 0)
        {
            enqueue(readyQueue, temp);
        }

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (!isEmpty(readyQueue))
            {
                currentProcess = (ProcessInfo *)dequeue(readyQueue);
                prevStartTime = getClk();
                runProcess(currentProcess);
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                currentProcess->startTime = getClk();
                logProcess(currentProcess, getClk(), STARTED);
            }
        }

        // if process done or quantum ended
        if (currentProcess != NULL)
        {

            // If process Done
            if (currentProcess->remainingTime <= 0)
            {
                // end
                if (currentProcess->state != FINISHED)
                {
                    count++;

                    currentProcess->endTime = getClk();
                    deallocateProcessMemory(currentProcess);
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess, getClk(), FINISHED);
                }

                if (!isEmpty(readyQueue))
                {
                    currentProcess = (ProcessInfo *)dequeue(readyQueue);
                    prevStartTime = getClk();
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

            // If Quantum Has ended
            else if ((getClk() >= prevStartTime + quantum) && !(currentProcess->remainingTime == 0))
            {

                pauseProcess(currentProcess);
                enqueue(readyQueue, currentProcess);
                currentProcess->lastStopTime = getClk();
                logProcess(currentProcess, getClk(), STOPPED);
                if (!isEmpty(readyQueue))
                {
                    currentProcess = (ProcessInfo *)dequeue(readyQueue);
                    prevStartTime = getClk();
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
    }
}

void clearScheduler(int sig)
{

    destroyClk(false, 2);

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

        destroyQueue(finishQueue);
    }
}

void deallocateProcessMemory(ProcessInfo* process) {
    int size = getNearestPowerOfTwo(process->memsize);
    deallocateMemory(memoryTreeRoot, process->address, size);
    recursiveMerge(memoryTreeRoot);
    processMemoryLog(process, getClk(), 0, size);
}