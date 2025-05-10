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
        execl("./bin/process.out", "./process.out", shmidSTR, NULL);
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

int CreateProcessInfoSHM_ID()
{
    return shmget(IPC_PRIVATE, sizeof(ProcessInfo), IPC_CREAT | 0666);
}

// semaphores

union Semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int initSemaphore(int pid)
{
    key_t semKey = ftok("sem", pid);
    if (semKey == -1)
    {
        perror("ftok for semaphore failed");
        exit(EXIT_FAILURE);
    }

    int semid = semget(semKey, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    union Semun sem_union;
    sem_union.val = 0; // binary semaphore: initially available
    if (semctl(semid, 0, SETVAL, sem_union) == -1)
    {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }

    return semid;
}

void down(int semid)
{
    struct sembuf op = {0, -1, 0}; // sem_num = 0 (only one)
    if (semop(semid, &op, 1) == -1)
    {
        perror("Semaphore down failed");
        exit(EXIT_FAILURE);
    }
}

void up(int semid)
{
    struct sembuf op = {0, 1, 0}; // sem_num = 0
    if (semop(semid, &op, 1) == -1)
    {
        perror("Semaphore up failed");
        exit(EXIT_FAILURE);
    }
}
void removeSemaphore(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("Failed to remove semaphore");
        exit(EXIT_FAILURE);
    }
}
