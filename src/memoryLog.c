#include "memoryLog.h"

//initialize the output file
void initMemoryLog() 
{
    FILE *fp = fopen("memory.log", "w");
    if (!fp) {
        perror("Failed to initialize memory.log");
        return;
    }

    fprintf(fp, "#At time x allocated y bytes for process z from i to j\n");
    fclose(fp);
}
// Logs memory info once a process is allocated or deallocated 
void processMemoryLog(ProcessInfo* process, int timestep, int action, int size) // action = 1 for allocation & 0 for deallocation.. size is the size reserved in memory (powers of two)
{ 
    FILE *fp = fopen("memory.log", "a");
    if (!fp) {
        perror("Failed to log in memory.log");
        return;
    }

    if (action) 
    {
        fprintf(fp, "At time %d allocated %d bytes for process %d from %d to %d\n", timestep, process->memsize, 
            process->pid, process->address, process->address + size - 1);
    }
    else 
    {
        fprintf(fp, "At time %d freed %d bytes from process %d from %d to %d\n", timestep, process->memsize, 
            process->pid, process->address, process->address + size - 1);
    }

    fclose(fp);
}