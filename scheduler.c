#include "headers.h"

int main(int argc, char *argv[])
{
    initClk();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    destroyClk(true);
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