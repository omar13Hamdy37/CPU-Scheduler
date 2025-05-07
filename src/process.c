#include "headers.h"
#include "../models/process_info.h"
/* Modify this file as needed*/
int remainingtime;
void clearProcess();
ProcessInfo *shm_ptr ;

int main(int agrc, char *argv[])
{
    signal(SIGINT, clearProcess);
    initClk();
    // get shmid passed as an argument
    int shmid = atoi(argv[1]);
    // get shared memory pointer
    shm_ptr = (ProcessInfo *)shmat(shmid, NULL, 0);

    int prevClk = getClk();

    while (shm_ptr->remainingTime > 0)
    {
        int currClk = getClk();

        if (currClk > prevClk)
        {

            (shm_ptr->remainingTime)--;

            prevClk = currClk;
        }
    }

    clearProcess();

    return 0;
}

void clearProcess()
{
    printf("CLEAR PROCESS CALLED\n");
    destroyClk(false);
    shmdt(shm_ptr);
}