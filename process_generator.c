#include "headers.h"
#include "queue.h"

Queue* processQueue; // Queue of All Processes loaded from inputFile

void clearResources(int);
void loadInputFile(char* fileName);

typedef struct ProcessInfo { // typedef so I dont always have to type struct ProcessInfo
    int pid; // useful for scheduler to control process.
    int arrivalTime; // useful for process_generator, scheduler, stats.
    int startTime; // useful for stats
    int runTime; // useful for scheduler, process itself, stats
    int endTime; // useful for stats
    int waitingTime; // useful for stats
    int priority; // useful for scheduling algorithms
    
    int remainingTime; // useful for process itself, scheduler
    int turnaroundTime; // useful for stats
    int weightedTurnaroundTime; // useful for stats
} ProcessInfo;

int AllocateProcess(int time, ProcessInfo** process) {
    ProcessInfo* nextProcess = (ProcessInfo*)peek(processQueue);
    if (nextProcess == NULL) return 0;
    if (nextProcess->arrivalTime == time) {
        *process = (ProcessInfo*)dequeue(processQueue);
        return 1; 
    }

    return 0; 
}

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    processQueue = createQueue(); // Queue Creation
    loadInputFile("processes.txt");
    
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    // 3. Initiate and create the scheduler and clock processes.
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    int timestep;
    // TODO Generation Main Loop
    while (!isEmpty(processQueue)) {   // To Be revised later (While or do while) loop
        timestep = getClk();
        ProcessInfo* process = NULL;

        while (AllocateProcess(timestep, &process)) {
            /*
            ...
            */
        }
    }
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void loadInputFile(char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    char line[256];
    fgets(line, sizeof(line), file); // skipping the comment line

    int id, arrivalTime, runTime, priority;
    while (fscanf(file, "%d\t%d\t%d\t%d", &id, &arrivalTime, &runTime, &priority) != EOF) {
        ProcessInfo* process = (ProcessInfo*)malloc(sizeof(ProcessInfo));
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

        enqueue(processQueue, process);
    }
    fclose(file);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
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
