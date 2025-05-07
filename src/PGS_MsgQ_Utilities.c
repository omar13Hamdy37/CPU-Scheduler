
#include "PGS_MsgQ_Utilities.h"

// Create msg queue and returns the msg queue id
int createPGSchedulerMsgQueue()
{
    key_t key = ftok("MSQKEY", 1);

    int msqid = msgget(key, IPC_CREAT | 0666); // Creates a message queue if it doesn't exist.
    return msqid;
}

// Sends process to scheduler using message queue. returns -1 if failed.
int SendToScheduler(ProcessInfo processInfo, int msqid)
{
    PGSchedulerMsgBuffer MsgToScheduler;
    MsgToScheduler.mtype = 1;
    MsgToScheduler.processInfo = processInfo;

    return msgsnd(msqid, &MsgToScheduler, sizeof(MsgToScheduler.processInfo), !IPC_NOWAIT);
}
// msgFromPG passed will now contain message. returns -1 if no message in queue.
int ReceiveFromPG(PGSchedulerMsgBuffer *msgFromPG, int msqid)
{
    return msgrcv(msqid, msgFromPG, sizeof(msgFromPG->processInfo), 1, IPC_NOWAIT);
}

// msgFromPG passed will now contain message. BLOCKS till a msg is sent

int BlockingReceiveFromPG(PGSchedulerMsgBuffer *msgFromPG, int msqid)
{
    return msgrcv(msqid, msgFromPG, sizeof(msgFromPG->processInfo), 1, 0);
}

// unpacks msgFromPG to get processInfo
ProcessInfo UnpackMsgBuffer(PGSchedulerMsgBuffer msgFromPG)
{
    return msgFromPG.processInfo;
}

void ClearMsgQ(int msqid)
{
    if (msqid != -1)
    {
        msgctl(msqid, IPC_RMID, NULL);
    }
}