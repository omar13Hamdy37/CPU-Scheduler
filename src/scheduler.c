
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

// if done sig is sent and we change that bool

void handle_Pg_Done(int sig)
{
    if (sig == PgSentAllProcesses)
    {
        PgDone = true;
        printf("SIGNAL RECEIVED\n");
    }
}

void runSRTN_new(int msqid);
void runHPF_new(int msqid);
void runRR_new(int msqid, int quantum);

int totalNumProcesses ;
int msqid; // id of message queue we use to communicate with PG

int main(int argc, char *argv[])
{


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
    int quantum = atoi(argv[3]);
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
        runRR_new(msqid, quantum);
        break;
    default:
        printf("Invalid scheduling algorithm selected.\n");
        exit(1);
    }
    destroyClk(true);
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

// Keep these functions
void runSRTN_new(int msqid)
{
    int count = 0;
    // TODO: CHECK THESE INIT ARE OK
    // initalize clock
    initClk();
    int time = getClk();

    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    // queue for ready processes
    // pri queue and srt is the priority thing
    PriQueue *priReadyQueue = createPriQueue();

    Queue *finishQueue = createQueue();

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
                logProcess(currentProcess,getClk(),STARTED);

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
                logProcess(currentProcess,getClk(),STOPPED);


                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {
                    runProcess(currentProcess);
                    ProcessState state;
                    if(currentProcess->state == STOPPED)
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


                    logProcess(currentProcess,getClk(),state);

                }
            }
        }

        // if process done
        if (currentProcess != NULL)
        {

            if (currentProcess->remainingTime <= 0)
            {
                // end
                if(currentProcess->state != FINISHED)
                {
                count++;

                currentProcess->endTime = getClk();
                enqueue(finishQueue, currentProcess);
                logProcess(currentProcess,getClk(),FINISHED);

                }

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {

                    runProcess(currentProcess);
                    ProcessState state;
                    if(currentProcess->state == STOPPED)
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


                    logProcess(currentProcess,getClk(),state);
                }
            }
        }

    }
    printPointersQueue(finishQueue,printProcessInfoPtr);
    schedulerPerf(finishQueue);


}
void runHPF_new(int msqid) {
    int count = 0;
    // initalize clock
    initClk();
    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    // queue for ready processes
    // pri queue and pri is the priority thing
    PriQueue *priReadyQueue = createPriQueue();

    Queue *finishQueue = createQueue();

    // TODO: FIX WHILE LOOP CONDITION
    // 3ayz while pg not done or queue not empty
    ProcessInfo *currentProcess = NULL;
    int *pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {
        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);

            int shmid = CreateProcessInfoSHM_ID();

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

            *process = unpackedProcess;
            process->shmid = shmid;

            // TODO: REMOVE LATER (FOR NOW CHECK THAT PROCESS COMES AT CORRECT TIME)

            printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
                   process->pid, getClk(), process->arrivalTime);
            // negative priority so pri has highest prioity
            enqueuePri(priReadyQueue, process, -(process->priority));
            
        }

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
            {

                runProcess(currentProcess);
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                currentProcess->startTime = getClk();
                logProcess(currentProcess,getClk(),STARTED);

            }
        }

        // if process done
        if (currentProcess != NULL)
        {

            if (currentProcess->remainingTime <= 0)
            {
                // end
                if(currentProcess->state != FINISHED)
                {
                    count++;

                    currentProcess->endTime = getClk();
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess,getClk(),FINISHED);
                }

                if (dequeuePri(priReadyQueue, (void **)&currentProcess, pri) != 0)
                {

                    runProcess(currentProcess);
                    currentProcess->startTime = getClk();
                    currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;

                    logProcess(currentProcess,getClk(),STARTED);
                }
            }
        }

    }
    schedulerPerf(finishQueue);
}

void runRR_new(int msqid, int quantum) {
    int count = 0;
    // initalize clock
    initClk();
    // initalize scheduler to be able to scheduler
    initSchedulerLog();
    // Prepare message buffer
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;

    // Time at which current process started its runningTime 
    int prevStartTime;
    // queue for ready processes
    // pri queue and pri is the priority thing (used for sorting processes with same arrival time by their priority)
    PriQueue *priReadyQueue = createPriQueue();
    // Actual readyQueue for RR
    Queue *ReadyQueue = createQueue();
    Queue *finishQueue = createQueue();

    // TODO: FIX WHILE LOOP CONDITION
    // 3ayz while pg not done or queue not empty
    ProcessInfo *currentProcess = NULL;
    ProcessInfo *temp = NULL;
    int *pri = malloc(sizeof(int));
    while (count != totalNumProcesses)
    {
        // Keep on extracting from msg queue till its empty
        // must do that in case multiple processes have the same arrival time
        while (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);

            int shmid = CreateProcessInfoSHM_ID();

            ProcessInfo *process = (ProcessInfo *)shmat(shmid, NULL, 0);

            *process = unpackedProcess;
            process->shmid = shmid;

            // TODO: REMOVE LATER (FOR NOW CHECK THAT PROCESS COMES AT CORRECT TIME)

            printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
                   process->pid, getClk(), process->arrivalTime);
            // negative priority so pri has highest prioity
            enqueuePri(priReadyQueue, process, -(process->priority));  
        }
        // dequeue the sorted process to the actual RR ready queue
        while (dequeuePri(priReadyQueue, (void **)&temp, pri) != 0) {
            enqueue(ReadyQueue, temp);
        }

        // If you're the first process you can run
        if (currentProcess == NULL)
        {

            if (!isEmpty(ReadyQueue))
            {
                currentProcess = (ProcessInfo *)dequeue(ReadyQueue);
                prevStartTime = getClk();
                runProcess(currentProcess);
                currentProcess->waitingTime += getClk() - currentProcess->arrivalTime;
                currentProcess->startTime = getClk();
                logProcess(currentProcess,getClk(),STARTED);

            }
        }

        // if process done or quantum ended
        if (currentProcess != NULL)
        {
            // process will run min(quantum, remainingtime)
            int runningTime;
            if (quantum < currentProcess->remainingTime) runningTime = quantum;
            else runningTime = currentProcess->remainingTime;
            
            // If process Done
            if (currentProcess->remainingTime <= 0)
            {
                // end
                if(currentProcess->state != FINISHED)
                {
                    count++;

                    currentProcess->endTime = getClk();
                    enqueue(finishQueue, currentProcess);
                    logProcess(currentProcess,getClk(),FINISHED);
                }

                if (!isEmpty(ReadyQueue))
                {
                    currentProcess = (ProcessInfo *)dequeue(ReadyQueue);
                    prevStartTime = getClk();
                    runProcess(currentProcess);
                    ProcessState state;
                    if(currentProcess->state == STOPPED)
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


                    logProcess(currentProcess,getClk(),state);
                }
            }

            // If Quantum Has ended
            else if (getClk() >= prevStartTime + quantum) {

                pauseProcess(currentProcess);
                enqueue(ReadyQueue, currentProcess);
                currentProcess->lastStopTime = getClk();
                logProcess(currentProcess,getClk(),STOPPED);
                if (!isEmpty(ReadyQueue))
                {
                    currentProcess = (ProcessInfo *)dequeue(ReadyQueue);
                    prevStartTime = getClk();
                    runProcess(currentProcess);
                    ProcessState state;
                    if(currentProcess->state == STOPPED)
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


                    logProcess(currentProcess,getClk(),state);

                }
            } 
        }

    }
    schedulerPerf(finishQueue);
}

// void runHPF(int msqid)
// {

//     initSchedulerLog(); // Initialize the log file

//     Queue *readyQueue = createQueue(); // Create linked list queue
//     PGSchedulerMsgBuffer msgBuffer;

//     int timestep = 0;
//     float totalCPUTime = 0;
//     float totalTurnaroundTime = 0;
//     float totalWaitingTime = 0;
//     float totalWTA = 0;
//     int numCompletedProcesses = 0;
//     float squaredDiffWTA = 0;

//     while (true)
//     {
//         // Receive new processes
//         if (ReceiveFromPG(&msgBuffer, msqid) != -1)
//         {
//             ProcessInfo *newProcess = (ProcessInfo *)malloc(sizeof(ProcessInfo));
//             *newProcess = UnpackMsgBuffer(msgBuffer);
//             enqueue(readyQueue, newProcess);
//         }

//         // If there is a ready process
//         // TODO CHECK THAT READY QUEUE NOT EMPTY AND PG NOT DONE
//         if (!isEmpty(readyQueue))
//         {
//             ProcessInfo *selectedProcess = dequeue_highest_priority(readyQueue);

//             int pid = fork();
//             if (pid == 0)
//             {
//                 // Child process: execute "process"
//                 char runtimeStr[10];
//                 sprintf(runtimeStr, "%d", selectedProcess->runTime);
//                 execl("./process", "process", runtimeStr, (char *)NULL);
//                 perror("Failed to exec process");
//                 exit(EXIT_FAILURE);
//             }
//             else if (pid > 0)
//             {
//                 // Parent process: Scheduler
//                 selectedProcess->pid = pid;
//                 selectedProcess->startTime = timestep;
//                 selectedProcess->state = STARTED;

//                 logProcess(selectedProcess, timestep, STARTED);

//                 // Simulate running for runTime units
//                 for (int i = 0; i < selectedProcess->runTime; i++)
//                 {
//                     usleep(1000);
//                     timestep++;
//                     totalCPUTime++;
//                 }

//                 // Wait for the child to finish
//                 waitpid(pid, NULL, 0);

//                 selectedProcess->endTime = timestep;
//                 selectedProcess->turnaroundTime = selectedProcess->endTime - selectedProcess->arrivalTime;
//                 selectedProcess->waitingTime = selectedProcess->turnaroundTime - selectedProcess->runTime;
//                 selectedProcess->weightedTurnaroundTime = (float)selectedProcess->turnaroundTime / selectedProcess->runTime;
//                 selectedProcess->state = FINISHED;

//                 totalTurnaroundTime += selectedProcess->turnaroundTime;
//                 totalWaitingTime += selectedProcess->waitingTime;
//                 totalWTA += selectedProcess->weightedTurnaroundTime;
//                 squaredDiffWTA += selectedProcess->weightedTurnaroundTime * selectedProcess->weightedTurnaroundTime;
//                 numCompletedProcesses++;

//                 logProcess(selectedProcess, timestep, FINISHED);

//                 free(selectedProcess); // Free the memory
//             }
//             else
//             {
//                 perror("Failed to fork");
//             }
//         }
//         if (isEmpty(readyQueue))
//         {
//             break;
//         }
//         usleep(1000); // simulate 1ms timestep
//         timestep++;
//     }
//     float avgWTA = totalWTA / numCompletedProcesses;
//     float avgWaitingTime = totalWaitingTime / numCompletedProcesses;
//     float stdDevWTA = sqrt(squaredDiffWTA / numCompletedProcesses - avgWTA * avgWTA);
//     float cpuUtilization = (totalCPUTime / timestep) * 100;

//     destroyQueue(readyQueue); // If you later add termination condition
// }