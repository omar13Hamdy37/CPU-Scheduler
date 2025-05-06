#include "../include/scheduler_algo.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include "../include/PGS_MsgQ_Utilities.h"
#include "queue.h"

#include <signal.h>


#define TRUE 1

// signal make pg_Done = true


void runHPF(int msqid)
{


    initSchedulerLog(); // Initialize the log file

    Queue *readyQueue = createQueue(); // Create linked list queue
    PGSchedulerMsgBuffer msgBuffer;

    int timestep = 0;
    float totalCPUTime = 0;
    float totalTurnaroundTime = 0;
    float totalWaitingTime = 0;
    float totalWTA = 0;
    int numCompletedProcesses = 0;
    float squaredDiffWTA = 0;

    while (TRUE)
    {
        // Receive new processes
        if (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo *newProcess = (ProcessInfo *)malloc(sizeof(ProcessInfo));
            *newProcess = UnpackMsgBuffer(msgBuffer);
            enqueue(readyQueue, newProcess);
        }

        // If there is a ready process
        // TODO CHECK THAT READY QUEUE NOT EMPTY AND PG NOT DONE
        if (!isEmpty(readyQueue))
        {
            ProcessInfo *selectedProcess = dequeue_highest_priority(readyQueue);

            int pid = fork();
            if (pid == 0)
            {
                // Child process: execute "process"
                char runtimeStr[10];
                sprintf(runtimeStr, "%d", selectedProcess->runTime);
                execl("./process", "process", runtimeStr, (char *)NULL);
                perror("Failed to exec process");
                exit(EXIT_FAILURE);
            }
            else if (pid > 0)
            {
                // Parent process: Scheduler
                selectedProcess->pid = pid;
                selectedProcess->startTime = timestep;
                selectedProcess->state = STARTED;

                logProcess(selectedProcess, timestep, STARTED);

                // Simulate running for runTime units
                for (int i = 0; i < selectedProcess->runTime; i++)
                {
                    usleep(1000);
                    timestep++;
                    totalCPUTime++;
                }

                // Wait for the child to finish
                waitpid(pid, NULL, 0);

                selectedProcess->endTime = timestep;
                selectedProcess->turnaroundTime = selectedProcess->endTime - selectedProcess->arrivalTime;
                selectedProcess->waitingTime = selectedProcess->turnaroundTime - selectedProcess->runTime;
                selectedProcess->weightedTurnaroundTime = (float)selectedProcess->turnaroundTime / selectedProcess->runTime;
                selectedProcess->state = FINISHED;

                totalTurnaroundTime += selectedProcess->turnaroundTime;
                totalWaitingTime += selectedProcess->waitingTime;
                totalWTA += selectedProcess->weightedTurnaroundTime;
                squaredDiffWTA += selectedProcess->weightedTurnaroundTime * selectedProcess->weightedTurnaroundTime;
                numCompletedProcesses++;

                logProcess(selectedProcess, timestep, FINISHED);

                free(selectedProcess); // Free the memory
            }
            else
            {
                perror("Failed to fork");
            }
        }
        if (isEmpty(readyQueue))
        {
            break;
        }
        usleep(1000); // simulate 1ms timestep
        timestep++;
    }
    float avgWTA = totalWTA / numCompletedProcesses;
    float avgWaitingTime = totalWaitingTime / numCompletedProcesses;
    float stdDevWTA = sqrt(squaredDiffWTA / numCompletedProcesses - avgWTA * avgWTA);
    float cpuUtilization = (totalCPUTime / timestep) * 100;

    destroyQueue(readyQueue); // If you later add termination condition
}

ProcessInfo *dequeue_highest_priority(Queue *q)
{
    if (isEmpty(q))
        return NULL;

    QueueNode *curr = q->front;
    QueueNode *prev = NULL;
    QueueNode *minPrev = NULL;
    QueueNode *minNode = curr;

    ProcessInfo *minProcess = (ProcessInfo *)curr->data;

    while (curr != NULL && curr->next != NULL)
    {
        ProcessInfo *nextProcess = (ProcessInfo *)(curr->next->data);
        if (nextProcess->priority < minProcess->priority)
        {
            minProcess = nextProcess;
            minPrev = curr;
            minNode = curr->next;
        }
        curr = curr->next;
    }

    // Remove minNode from queue
    if (minPrev == NULL)
    {
        // minNode is front
        q->front = minNode->next;
        if (q->rear == minNode)
            q->rear = NULL;
    }
    else
    {
        minPrev->next = minNode->next;
        if (q->rear == minNode)
            q->rear = minPrev;
    }

    ProcessInfo *selectedProcess = (ProcessInfo *)(minNode->data);
    free(minNode);

    return selectedProcess;
}

void runProcess(ProcessInfo *process)
{
    int pid = fork();
    if (pid == 0)
    {
        char runtimeStr[10];
        sprintf(runtimeStr, "%d", process->runTime);
        execl("./process.out", "./process.out", runtimeStr, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        process->pid = pid;
    }
}

void pauseProcess(pid_t pid)
{
    kill(pid, SIGSTOP);
}

void resumeProcess(pid_t pid)
{
    kill(pid, SIGCONT);
}

void stopProcess(pid_t pid)
{
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
}

ProcessInfo *dequeueShortestJob(Queue *q)
{
    if (isEmpty(q))
        return NULL;

    QueueNode *curr = q->front;
    QueueNode *minNode = curr;
    while (curr != NULL)
    {
        ProcessInfo *pCurr = (ProcessInfo *)curr->data;
        ProcessInfo *pMin = (ProcessInfo *)minNode->data;
        if (pCurr->remainingTime < pMin->remainingTime)
        {
            minNode = curr;
        }
        curr = curr->next;
    }

    // Remove minNode from queue and return it
    ProcessInfo *selected = (ProcessInfo *)minNode->data;
    removeNode(q, minNode);
    return selected;
}

void runSRTN(int msqid)
{
    ProcessInfo *currentRunningProcess = NULL;
    PGSchedulerMsgBuffer msgBuffer;
    Queue *readyQueue = createQueue(); // ready queue for incoming processes
    int timestep = 0;

    float totalCPUTime = 0;
    float totalTurnaroundTime = 0;
    float totalWaitingTime = 0;
    float totalWTA = 0;
    int numCompletedProcesses = 0; // Total no. of processes completed
    float squaredDiffWTA = 0;      // For calc standard deviation

    while (1)
    {
        if (ReceiveFromPG(&msgBuffer, msqid) != -1)
        {
            ProcessInfo *newProcess = malloc(sizeof(ProcessInfo));
            *newProcess = UnpackMsgBuffer(msgBuffer);
            enqueue(readyQueue, newProcess);

            // Check if preemption is needed
            if (currentRunningProcess == NULL)
            {
                // No process running -> start the shortest process
                currentRunningProcess = dequeueShortestJob(readyQueue);
                if (currentRunningProcess != NULL)
                {
                    runProcess(currentRunningProcess);
                    logProcess(currentRunningProcess, timestep, STARTED);
                }
            }
            else if (newProcess->remainingTime < currentRunningProcess->remainingTime)
            {
                // Preempt current process
                pauseProcess(currentRunningProcess->pid);
                enqueue(readyQueue, currentRunningProcess);

                // Select new shortest
                currentRunningProcess = dequeueShortestJob(readyQueue);
                if (currentRunningProcess->pid > 0)
                {
                    resumeProcess(currentRunningProcess->pid);
                    logProcess(currentRunningProcess, timestep, RESUMED);
                }
                else
                {
                    runProcess(currentRunningProcess);
                    logProcess(currentRunningProcess, timestep, STARTED);
                }
            }
        }

        // Check if current process finished
        if (currentRunningProcess != NULL)
        {
            int status;
            pid_t finishedPID = waitpid(currentRunningProcess->pid, &status, WNOHANG);
            if (finishedPID == currentRunningProcess->pid)
            {
                logProcess(currentRunningProcess, timestep, FINISHED);

                currentRunningProcess->endTime = timestep;
                currentRunningProcess->turnaroundTime = currentRunningProcess->endTime - currentRunningProcess->arrivalTime;
                currentRunningProcess->waitingTime = currentRunningProcess->turnaroundTime - currentRunningProcess->runTime;
                currentRunningProcess->weightedTurnaroundTime = (float)currentRunningProcess->turnaroundTime / currentRunningProcess->runTime;

                totalTurnaroundTime += currentRunningProcess->turnaroundTime;
                totalWaitingTime += currentRunningProcess->waitingTime;
                totalWTA += currentRunningProcess->weightedTurnaroundTime;
                squaredDiffWTA += currentRunningProcess->weightedTurnaroundTime * currentRunningProcess->weightedTurnaroundTime;
                numCompletedProcesses++;

                free(currentRunningProcess);
                currentRunningProcess = NULL;

                // Start next process if ready
                currentRunningProcess = dequeueShortestJob(readyQueue);
                if (currentRunningProcess != NULL)
                {
                    if (currentRunningProcess->pid > 0)
                    {
                        resumeProcess(currentRunningProcess->pid);
                        logProcess(currentRunningProcess, timestep, RESUMED);
                    }
                    else
                    {
                        runProcess(currentRunningProcess);
                        logProcess(currentRunningProcess, timestep, STARTED);
                    }
                }
            }
        }

        if (currentRunningProcess != NULL)
        {
            currentRunningProcess->remainingTime -= 1;
            totalCPUTime += 1;
        }

        if (isEmpty(readyQueue) && currentRunningProcess == NULL)
        {
            break;
        }

        usleep(1000); // simulate time step
        timestep++;
    }
    float avgWTA = totalWTA / numCompletedProcesses;
    float avgWaitingTime = totalWaitingTime / numCompletedProcesses;
    float stdDevWTA = sqrt(squaredDiffWTA / numCompletedProcesses - avgWTA * avgWTA);
    float cpuUtilization = (totalCPUTime / timestep) * 100;

    destroyQueue(readyQueue);
}
