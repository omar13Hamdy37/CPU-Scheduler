#include "Scheduler_log.h"

void initSchedulerLog() {
    FILE *fp = fopen("scheduler.log", "w");
    if (!fp) {
        perror("Failed to initialize scheduler.log");
        return;
    }

    fprintf(fp, "#At time x process y state arr w total z remain y wait k\n");
    fclose(fp);
}

void logProcess (ProcessInfo* process, int timestep, ProcessState newState) {

    process->state = newState;

    FILE *fp = fopen("scheduler.log", "a");
    if (!fp) {
        perror("Failed to open scheduler.log");
        return;
    }

    int pid = process->pid;
    int arriveTime = process->arrivalTime;
    int runTime = process->runTime;
    int remainingTime = process->remainingTime;
    int waitingTime = process->waitingTime;
    int TA = process->turnaroundTime;
    float WTA = process->weightedTurnaroundTime;

    switch (newState) {
        case STARTED:
            fprintf(fp, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                    timestep, pid, arriveTime, runTime, remainingTime, waitingTime);
            break;
        case STOPPED:
            fprintf(fp, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                    timestep, pid, arriveTime, runTime, remainingTime, waitingTime);
            break;
        case RESUMED:
            fprintf(fp, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                    timestep, pid, arriveTime, runTime, remainingTime, waitingTime);
            break;
        case FINISHED:
            TA = process->endTime - process->arrivalTime;
            WTA = (float)TA / runTime;
            process->turnaroundTime = TA;
            process->weightedTurnaroundTime = WTA;
            fprintf(fp, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                    timestep, pid, arriveTime, runTime, remainingTime, waitingTime, TA, WTA);
            break;
    }

    fclose(fp);
      
}