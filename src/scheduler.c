#include "headers.h"
#include "../models/scheduling_types.h"
#include "PGS_MsgQ_Utilities.h"
#include "Scheduler_log.h"
#include <stdlib.h> // for atoi
#include "queue.h"

Queue *readyQueue; // queue of ready processes info
int msqid;         // id of message queue we use to communicate with PG

int main(int argc, char *argv[])
{
    // initalize clock
    initClk();
    // get scheduling type from arguments ( we convert argument to int as it was string then to enum)
    enum scheduling_type algorithm = (enum scheduling_type)atoi(argv[1]);
    // Print to check algorithm choice correct
    switch (algorithm)
    {
    case HPF:
        printf("Scheduling Algorithm is: Non-preemptive Highest Priority First (HPF)\n");
        break;
    case SRTN:
        printf("Scheduling Algorithm is: Shortest Remaining Time Next (SRTN)\n");
        break;
    case RR:
        printf("Scheduling Algorithm is: Round Robin (RR)\n");
        break;
    default:
        printf("Invalid scheduling algorithm selected.\n");
        exit(1);
    }
    // Get msqid
    msqid = createPGSchedulerMsgQueue();
    if (msqid == -1)
    {
        printf("Error getting msqid\n");
        exit(1);
    }

    // Create queue of ready processes
    readyQueue = createQueue();

    // Initiate the Scheduler.log file 
    initSchedulerLog();

    // message buffer and send type to 1
    PGSchedulerMsgBuffer msgBuffer;
    msgBuffer.mtype = 1;
    while (true)
    {
        // Recieve message
        ReceiveFromPG(&msgBuffer, msqid);
        // Unpack to get process info
        ProcessInfo unpackedProcess = UnpackMsgBuffer(msgBuffer);
        // create process info (dynamic as to avoid rewrite)
        ProcessInfo *processCopy = (ProcessInfo *)malloc(sizeof(ProcessInfo));
        *processCopy = unpackedProcess;

        // enqueue that process 
        enqueue(readyQueue, processCopy);
        
        // print to check ready queue is correct
        printf("Process with id %i is ready at timestep %i, arrival time should be %i\n",
               processCopy->pid, getClk(), processCopy->arrivalTime);
        
        // Simple example of how to use the logProcess function...
        // make sure to call the function each time a process changes its state 
        // parameters->[process, current time, newState of the process]
        logProcess(processCopy, getClk(), STARTED);
    }

    // TODO implement the scheduler :)
    // ana edetak aho the type of scheduling algo check models hatal2y enum
    // upon termination release the clock resources.

    // LATER destroyClk(true);
}
/*
1) The scheduler needs to have these data ready at the end. So here is what data you need to keep track of:
    a) CPU UTILIZATION -> time cpu used, total time
    b) average weighted turnaround time -> when process is done it calculates the turnaround time and adds it to the total
    c) average waiting time -> when process is done it calculates the waiting time and adds it to the total
    d) Standard deviation for average weighted turnaround time -> u have the info to calculate it

2) So when a process is done it sends a message to the scheduler with the data it needs to calculate the above.
3) How does the process send that data to the scheduler? Probably using a message queue or signal.
*/