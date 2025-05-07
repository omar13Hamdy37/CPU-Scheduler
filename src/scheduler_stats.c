#include "../include/scheduler_stats.h"
// max of two numbers usefull for calculating stats
#define max(a, b) ((a) > (b) ? (a) : (b))

// Creates the scheduler.perf file  --> just pass the finishedQueue 
void schedulerPerf(Queue* finishedQueue) {
    int CPU;
    float AvgWTA, AvgWaiting, stddev;
    calcStatistics(finishedQueue, &CPU, &AvgWTA, &AvgWaiting, &stddev);

    FILE *file = fopen("scheduler.perf", "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "CPU utilization = %d%%\n", CPU); // %% -> %
    fprintf(file, "Avg WTA = %.2f\n", AvgWTA);
    fprintf(file, "Avg Waiting = %.2f\n", AvgWaiting);
    fprintf(file, "Std WTA = %.2f\n", stddev);

    fclose(file);
}

void calcStatistics(Queue* finishedQueue, int* CPU, float* AvgWTA, float* AvgWaiting, float* stddev) {
    int totalTime = INT_MIN;
    int totalRunTime = 0;
    float totalWTA = 0;
    int totalWait = 0;
    int processesCount = 0;
    
    // For standard Deviation calculation (no need to create a new queue)
    float sum = 0;
    float sumSq = 0;
    QueueNode* current = finishedQueue->front;

    if (current != NULL)

    {
        int count = getQueueCount(finishedQueue);
        for (int i = 0; i < count; i++)
        {
        ProcessInfo* process = (ProcessInfo*)current->data;
        
        // Update max finish time
        totalTime = max(totalTime, process->endTime);
        
        // Sum total run time and total wait time
        totalRunTime += process->runTime;
        totalWait += process->waitingTime;

        // Calculate WTA and accumulate total WTA
        float WTA = (float)(process->endTime - process->arrivalTime) / process->runTime;
        process->weightedTurnaroundTime = WTA;
        totalWTA += WTA;

        // For stddev calculation
        sum += WTA;
        sumSq += WTA * WTA;
        
        // Process count
        processesCount++;
        // Move to next process
        current = current->next;
        }
    }

    // Calculate CPU utilization
    float Utilization = (float)totalRunTime / totalTime;
    *CPU = (int)(100 * Utilization);

    // Calculate averages
    *AvgWTA = totalWTA / processesCount;
    *AvgWaiting = (float)totalWait / processesCount;

    // Calculate standard deviation
    *stddev = sqrt((sumSq / processesCount) - (pow(sum / processesCount, 2)));
}
