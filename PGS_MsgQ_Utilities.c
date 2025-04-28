#include "headers.h"



// struct of msg buffer between PG and Scheduler
struct PGSchedulerMsgBuffer
{
    long mtype;      
    struct ProcessInfo processInfo; // Message data
};

int createPGSchedulerMsgQueue()
{
    key_t key = ftok("keyfile", 1);
    if (key == -1)
    {
        perror("ftok failed\n");

    }
    int msqid = msgget(key, IPC_CREAT | 0666); // Creates a message queue if it doesn't exist.
    if (msqid == -1)
    {
        perror("msgget failed\n");

    }
    return msqid;

}

// Sends process to scheduler using message queue. returns -1 if failed.
int SendToScheduler(struct ProcessInfo processInfo, int msqid)
{
    struct PGSchedulerMsgBuffer MsgToScheduler;
    MsgToScheduler.mtype = 0;
    MsgToScheduler.processInfo = processInfo;

    return msgsnd(msqid, &processInfo, sizeof(MsgToScheduler.processInfo), !IPC_NOWAIT);
    

}
// msgFromPG passed will now contain message. returns -1 if unsuccessful.
int ReceiveFromPG(struct PGSchedulerMsgBuffer * msgFromPG , int msqid)
{
    return msgrcv(msqid, msgFromPG, sizeof(msgFromPG->processInfo), 0, !IPC_NOWAIT);
    
}

// unpacks msgFromPG to get processInfo
struct ProcessInfo UnpackMsgBuffer(struct PGSchedulerMsgBuffer  msgFromPG)
{
    return msgFromPG.processInfo;
}