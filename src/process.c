#include "headers.h"
#include "../models/process_info.h"
#include "./scheduler_helper.h"
/* Modify this file as needed*/


void clearProcess(int sig);

ProcessInfo *shm_ptr;

int main(int agrc, char *argv[])
{
    signal(SIGINT, clearProcess);
    initClk();
    // shared resource between process and scheduler
    // for scheduler to read teh remaining time and its info

    // get shmid passed as an argument
    int shmid = atoi(argv[1]);
    // get shared memory pointer
    shm_ptr = (ProcessInfo *)shmat(shmid, NULL, 0);

    // decrement to be done each second
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
    clearProcess(0);

    return 0;
}

void clearProcess(int sig)
{


    destroyClk(false, 3);
    shmdt(shm_ptr);
}