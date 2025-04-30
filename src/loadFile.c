#include "loadFile.h"
#include <stdlib.h>

// Reading processes from input file to queue
void loadInputFile(char *fileName, Queue *queue)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(1);
    }

    char line[256];
    // Skip the comment line
    fgets(line, sizeof(line), file); 

    int id, arrivalTime, runTime, priority;
    while (fscanf(file, "%d\t%d\t%d\t%d", &id, &arrivalTime, &runTime, &priority) != EOF)
    {
        ProcessInfo *process = (ProcessInfo *)malloc(sizeof(ProcessInfo));
        process->pid = id;
        process->arrivalTime = arrivalTime;
        process->runTime = runTime;
        process->priority = priority;
        process->remainingTime = runTime;
        process->startTime = -1;
        process->endTime = -1;
        process->waitingTime = 0;
        process->turnaroundTime = 0;
        process->weightedTurnaroundTime = 0;
        process->state = WAITING;
        enqueue(queue, process);
    }

    fclose(file);
}