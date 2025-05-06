
#ifndef PGS_MSG1_UTILITIES_H
#define PGS_MSG1_UTILITIES_H

#include "../models/process_info.h"
#include <sys/msg.h>

// msg buffer struct
typedef struct
{
    long mtype;
    ProcessInfo processInfo;
} PGSchedulerMsgBuffer;

// Function prototypes
int createPGSchedulerMsgQueue();
int SendToScheduler(ProcessInfo processInfo, int msqid);
int BlockingReceiveFromPG(PGSchedulerMsgBuffer *msgFromPG, int msqid);
int ReceiveFromPG(PGSchedulerMsgBuffer *msgFromPG, int msqid);
ProcessInfo UnpackMsgBuffer(PGSchedulerMsgBuffer msgFromPG);
void ClearMsgQ(int msqid);

#endif
