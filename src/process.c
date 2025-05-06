#include "headers.h"
#include "../models/process_info.h"
/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    // get shmid passed as an argument
    int shmid = atoi(argv[1]);
    // get shared memory pointer
    ProcessInfo* shm_ptr = (ProcessInfo*) shmat(shmid, NULL, 0);
    

    // int prevClk = getClk();
    // printf("here");

    // while (shm_ptr->remainingTime > 0)
    // {
    //     int currClk = getClk();
    //     // if time passes decrement remaining time
    //     if (currClk > prevClk)
    //     {

    //         printProcessInfoPtr(shm_ptr);
    //         printf("\n");


    //         (shm_ptr->remainingTime)--;

    //         prevClk = currClk;
    //     }
    // }
    
    destroyClk(false);
    
    return 0;
}
