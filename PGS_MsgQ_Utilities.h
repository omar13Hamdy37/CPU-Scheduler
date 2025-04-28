
#ifndef PGS_MSG1_UTILITIES_H
#define PGS_MSG1_UTILITIES_H

#include "headers.h"  // Because you use struct ProcessInfo

// Struct declaration
struct PGSchedulerMsgBuffer
{
    long mtype;
    struct ProcessInfo processInfo;
};

// Function prototypes
int createPGSchedulerMsgQueue();
int SendToScheduler(struct ProcessInfo processInfo, int msqid);
int ReceiveFromPG(struct PGSchedulerMsgBuffer *msgFromPG, int msqid);
struct ProcessInfo UnpackMsgBuffer(struct PGSchedulerMsgBuffer msgFromPG);

#endif
