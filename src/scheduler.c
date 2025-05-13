
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
#include <signal.h>
#include <asm-generic/signal-defs.h>
// if done sig is sent and we change that bool
#define PgSentAllProcesses SIGUSR1
// bool saying pg is done sending processes
bool PgDone = false;
void receiveMQ();
void importToReadyQueue();
void allocateMemToProcess();
void allocateMemToWaitingProcesses();

int *pri;
ProcessInfo *temp;
ProcessInfo *currentProcess = NULL;
ProcessInfo *incomingProcess = NULL;
enum scheduling_type algorithm;

void clearScheduler(int sig);
int *AllocPri;
// Prepare message buffer

// Queues that we will use
Queue *finishQueue;
Queue *readyQueue;
PriQueue *waitingQ; // for processes that we couldn't allocate memory for
Queue *tempQ;
PriQueue *priReadyQueue;
PriQueue *memoryAllocQueue;

ProcessInfo *allocP = NULL;

// Memory root
MemoryTreeNode *memoryTreeRoot;
// prepare msgBuffer
PGSchedulerMsgBuffer msgBuffer;

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
void deallocateProcessMemory(ProcessInfo *process);
// num processes
int totalNumProcesses;
int msqid; // id of message queue we use to communicate with PG

int main(int argc, char *argv[])
{
    // signal(SIGUSR2, handle_process_signal);
    signal(SIGINT, clearScheduler);

    signal(PgSentAllProcesses, handle_Pg_Done);
    // initalize clock
    initClk();
    // get scheduling type from arguments ( we convert argument to int as it was string then to enum)
    algorithm = (enum scheduling_type)atoi(argv[1]);
    totalNumProcesses = atoi(argv[2]);
    AllocPri = malloc(sizeof(int));
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
    msgBuffer.mtype = 1;

    // initalize to be able to log
    initSchedulerLog();
    initMemoryLog();

    // queue for ready processes

    // pri queue and srt is the priority thing
    priReadyQueue = createPriQueue();
    finishQueue = createQueue();
    waitingQ = createPriQueue();
    tempQ = createQueue();
    memoryAllocQueue = createPriQueue();
    allocP = NULL;

    currentProcess = NULL;
    incomingProcess = NULL;
    pri = malloc(sizeof(int)); // (not really used)

    while (count != totalNumProcesses)
    {

        // try to allocate memory for all processes in waitingQ
        allocateMemToWaitingProcesses();

        // keep on receiving processes from msg q till it's empty
        receiveMQ();

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
        else
        {
            // If you're not the first process

            // Check if your time is done
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
                // check if I can allocate memory to waiting processes
                allocateMemToWaitingProcesses();
                // first check if another process arrived
                receiveMQ();

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
            else
            {
                // if you're not the first process
                // and you're not done yet
                // wait for process to decrement and check if another process has shorter rtn

                down(currentProcess->semid);
                receiveMQ();
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
    }
}

void runHPF_new(int msqid)
{

    int count = 0;

    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    initMemoryLog();
    // Prepare message buffer
    msgBuffer.mtype = 1;
    // queue for ready processes
    // pri queue and pri is the priority thing
    priReadyQueue = createPriQueue();
    finishQueue = createQueue();
    waitingQ = createPriQueue();
    tempQ = createQueue();
    memoryAllocQueue = createPriQueue();
    allocP = NULL;

    currentProcess = NULL;
    int *pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {
        // try to allocate memory for all processes in waitingQ
        allocateMemToWaitingProcesses();
        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time

        receiveMQ();

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
        else
        {
            // not the first processes
            // and you're done

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
                // allocate memory for waiting processes
                allocateMemToWaitingProcesses();
                // so that you are updated
                receiveMQ();

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
    memoryAllocQueue = createPriQueue();
    allocP = NULL;
    // initalize output files to be able to log
    initSchedulerLog();
    initMemoryLog();
    // Prepare message buffer
    msgBuffer;
    msgBuffer.mtype = 1;

    // Time at which current process started its runningTime
    int prevStartTime;
    int prevRemainingTime;
    // queue for ready processes
    // pri queue and pri is the priority thing (used for sorting processes with same arrival time by their priority)
    priReadyQueue = createPriQueue();
    // Actual readyQueue for RR
    readyQueue = createQueue();
    finishQueue = createQueue();
    waitingQ = createPriQueue();
    tempQ = createQueue();

    // 3ayz while pg not done or queue not empty
    currentProcess = NULL;
    temp = NULL;
    pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {

        // try to allocate memory for all processes in waitingQ
        allocateMemToWaitingProcesses();

        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        receiveMQ();

        // dequeue the sorted process to the actual RR ready queue
        importToReadyQueue();

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (!isEmpty(readyQueue))
            {

                currentProcess = (ProcessInfo *)dequeue(readyQueue);
                prevStartTime = getClk();
                prevRemainingTime = currentProcess->runTime;
                runProcess(currentProcess);
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                currentProcess->startTime = getClk();
                logProcess(currentProcess, getClk(), STARTED);
            }
        }
        else
        {

            // if you're not the first process

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
                allocateMemToWaitingProcesses();
                
                receiveMQ();
                // dequeue the sorted process to the actual RR ready queue
                importToReadyQueue();

                if (!isEmpty(readyQueue))
                {
                    currentProcess = (ProcessInfo *)dequeue(readyQueue);
                    prevStartTime = getClk();
                    prevRemainingTime = currentProcess->remainingTime;
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

            else if ((currentProcess->remainingTime <= prevRemainingTime - quantum) && !(currentProcess->remainingTime == 0))
            {

                pauseProcess(currentProcess);

                receiveMQ();
                // dequeue the sorted process to the actual RR ready queue
                importToReadyQueue();
                enqueue(readyQueue, currentProcess);
                printf("Enqueued process %i, at time %i\n", currentProcess->pid, getClk());
                currentProcess->lastStopTime = getClk();
                logProcess(currentProcess, getClk(), STOPPED);

                if (!isEmpty(readyQueue))
                {

                    currentProcess = (ProcessInfo *)dequeue(readyQueue);
                    prevStartTime = getClk();
                    prevRemainingTime = currentProcess->remainingTime;
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
            removeSemaphore(proc->shmid);

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

void deallocateProcessMemory(ProcessInfo *process)
{
    int size = getNearestPowerOfTwo(process->memsize);
    deallocateMemory(memoryTreeRoot, process->address, size);
    recursiveMerge(memoryTreeRoot);
    processMemoryLog(process, getClk(), 0, size);
}

void receiveMQ()
{

    // collect all processes of this second into memoryALLOCQUEUE
    while (ReceiveFromPG(&msgBuffer, msqid) != -1)
    {

        // unpack from msg q
        ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);
        // get shmid for this process
        int shmid = CreateProcessInfoSHM_ID();
        int semid = initSemaphore(unpackedProcess.pid);
        // attach to the scheduler

        ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

        // copy proces to shared memory
        *process = unpackedProcess;
        // put shmid inside process
        process->shmid = shmid;
        process->semid = semid;

        printf("Process %i received at time %i, should be %i\n", process->pid, getClk(), process->arrivalTime);
        // put into memory alloc queue depending on priority
        switch (algorithm)
        {
        case HPF:

            enqueuePri(memoryAllocQueue, process, -(process->priority));

            break;
        case SRTN:
            enqueuePri(memoryAllocQueue, process, -(process->remainingTime));
            break;
        case RR:
            enqueuePri(memoryAllocQueue, process, -(process->priority));

            break;
        }

        // try to allocate memory to process
    }
    // now actually alloc the procdequeuePri(priReadyQueue, (void **)&currentProcess, pripriesses

    while (!isEmptyPri(memoryAllocQueue))
    {

        dequeuePri(memoryAllocQueue, (void **)&allocP, AllocPri);

        allocateMemToProcess(allocP);
    }
}
void importToReadyQueue()
{

    while (dequeuePri(priReadyQueue, (void **)&temp, pri) != 0)
    {

        enqueue(readyQueue, temp);
        printf("Enqueued process %i, at time %i\n", temp->pid, getClk());
    }
}

void allocateMemToProcess(ProcessInfo *process)
{
    int size = getNearestPowerOfTwo(process->memsize);
    // get best fit for process in the current memory tree
    MemoryTreeNode *currentBestFit = getBestFit(memoryTreeRoot, size);
    // if we couldn't allocate memory for process enqueue it to waiting Q
    if (currentBestFit == NULL)
    {
        switch (algorithm)
        {
        case HPF:
            enqueuePri(waitingQ, process, -(process->priority));

            break;
        case SRTN:
            enqueuePri(waitingQ, process, -(process->remainingTime));
            break;
        case RR:
            enqueuePri(waitingQ, process, -(process->priority));

            break;
        }
    }
    else
    {
        // get address of allocated process
        int address = allocateMemory(currentBestFit, process->memsize);
        process->address = address;
        // negative remaining time so srt has highest prioity
        switch (algorithm)
        {
        case HPF:
            enqueuePri(priReadyQueue, process, -(process->priority));

            break;
        case SRTN:
            enqueuePri(priReadyQueue, process, -(process->remainingTime));
            break;
        case RR:
            enqueuePri(priReadyQueue, process, -(process->priority));

            break;
        }

        processMemoryLog(process, getClk(), 1, size);
    }
}

void allocateMemToWaitingProcesses()
{
    while (!isEmptyPri(waitingQ))
    {
        dequeuePri(waitingQ, (void **)&allocP, AllocPri);

        int size = getNearestPowerOfTwo(allocP->memsize);
        MemoryTreeNode *currentBestFit = getBestFit(memoryTreeRoot, size);

        // if we couldnt find best fit return process to waitingqueue
        if (currentBestFit == NULL)
        {
            enqueue(tempQ, allocP);
        }
        else
        {
            // get address (start byte) of process memory
            int address = allocateMemory(currentBestFit, allocP->memsize);
            allocP->address = address;
            // enqueue to priorityQ
            switch (algorithm)
            {
            case HPF:
                enqueuePri(priReadyQueue, allocP, -(allocP->priority));
                break;
            case SRTN:
                enqueuePri(priReadyQueue, allocP, -(allocP->remainingTime));
                break;
            case RR:
                enqueuePri(priReadyQueue, allocP, -(allocP->priority));
                break;
            }
            processMemoryLog(allocP, getClk(), 1, size);
        }
    }
    // return processes from temp to waitingQ
    while (!isEmpty(tempQ))
    {
        ProcessInfo *process = (ProcessInfo *)dequeue(tempQ);
        switch (algorithm)
        {
        case HPF:
            enqueuePri(waitingQ, process, -(process->priority));
            break;
        case SRTN:
            enqueuePri(waitingQ, process, -(process->remainingTime));
            break;
        case RR:
            enqueuePri(waitingQ, process, -(process->priority));
            break;
        }
    }
}