
#include "PGS_MsgQ_Utilities.h"

// Create msg queue and returns the msg queue id
int createPGSchedulerMsgQueue()
{
    key_t key = ftok("newKey", 1);

    int msqid = msgget(key, IPC_CREAT | 0666); // Creates a message queue if it doesn't exist.
    return msqid;

}

// Sends process to scheduler using message queue. returns -1 if failed.
int SendToScheduler( ProcessInfo processInfo, int msqid)
{
     PGSchedulerMsgBuffer MsgToScheduler;
    MsgToScheduler.mtype = 1;
    MsgToScheduler.processInfo = processInfo;

    return msgsnd(msqid, &MsgToScheduler, sizeof(MsgToScheduler.processInfo), !IPC_NOWAIT);
    

}
// msgFromPG passed will now contain message. returns -1 if unsuccessful.
int ReceiveFromPG( PGSchedulerMsgBuffer * msgFromPG , int msqid)
{
    return msgrcv(msqid, msgFromPG, sizeof(msgFromPG->processInfo), 1, !IPC_NOWAIT);
    
}

// unpacks msgFromPG to get processInfo
 ProcessInfo UnpackMsgBuffer( PGSchedulerMsgBuffer  msgFromPG)
{
    return msgFromPG.processInfo;
}