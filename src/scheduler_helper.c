#include "./scheduler_helper.h"




// Function to run process (begin)
void runProcess(ProcessInfo *process)
{
    // we fork it (dw bec we fork each process will be on its own.. even tho we rn the same ./process)
    int pid = fork();
    if (pid == 0)
    {
        char shmidSTR[12];
        sprintf(shmidSTR, "%d", process->shmid);
        execl("./process.out", "./process.out", shmidSTR, NULL);
        // only prints if couldnt' execl
        perror("execl failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        process->actualPid = pid;
    }
}

void pauseProcess(ProcessInfo *process)
{
    kill(process->actualPid, SIGSTOP);
}

void resumeProcess(ProcessInfo *process)
{
    kill(process->actualPid, SIGCONT);
}

void stopProcess(ProcessInfo *process)
{
    kill(process->actualPid, SIGKILL);
    waitpid(process->actualPid, NULL, 0);
}
int CreateProcessInfoSHM_ID()
{
    return shmget(IPC_PRIVATE, sizeof(ProcessInfo), IPC_CREAT | 0666);

}