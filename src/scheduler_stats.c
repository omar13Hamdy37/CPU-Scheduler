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

// Calculates all required variables for scheduler.perf file
void calcStatistics(Queue* finishedQueue, int* CPU, float* AvgWTA, float* AvgWaiting, float* stddev) {
    
    // Total time will be the maximum finishTime of all processes
    int totalTime = INT_MIN;
    int totalRunTime = 0;
    float totalWTA = 0;
    int totalWait = 0;
    int processesCount = 0;
    
    // For standard Deviation calculation
    Queue *stdQueue = createQueue(); 
    ProcessInfo* process = NULL;
    while (!isEmpty(finishedQueue)) {
        process = (ProcessInfo *)dequeue(finishedQueue);
        totalTime = max(totalTime, process->endTime);
        totalRunTime += process->runTime;
        float WTA = (float)(process->endTime - process->arrivalTime) / process->runTime;
        // update WTA of process if it wasn't calculated
        process->weightedTurnaroundTime = WTA;
        totalWTA += WTA;
        totalWait += process->waitingTime;
        processesCount++;
        // to be processed again in stddev calculations
        enqueue(stdQueue, process);
    }

    float Utilization = (float)totalRunTime / totalTime;

    *CPU = (int)(100 * Utilization);
    *AvgWTA = totalWTA / processesCount;
    *AvgWaiting = (float)totalWait / processesCount;
    *stddev = calculateSTD(totalWTA, processesCount, stdQueue);

}

float calculateSTD(float totalWTA, int size, Queue* queue) {
    
    float stddev = 0;
    float mean = totalWTA / (float)size;
    ProcessInfo* process = NULL;
    while (!isEmpty(queue)) {
        process = (ProcessInfo *)dequeue(queue);
        stddev += pow(process->weightedTurnaroundTime - mean, 2);
    }
    stddev = sqrt(stddev / (size - 1));
    return stddev;
}