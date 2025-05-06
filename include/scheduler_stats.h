
#ifndef SCHEDULER_STATS_H
#define SCHEDULER_STATS_H

#include "queue.h"
#include "queue.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <../models/process_info.h>


void calcStatistics(Queue* finishedQueue, int* CPU, float* AvgWTA, float* AvgWaiting, float* stddev);
float calculateSTD(float totalWTA, int size, Queue* queue);
void schedulerPerf(Queue* finishedQueue);

#endif