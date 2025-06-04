# CPU Scheduler Project — Operating Systems

A simulation of three CPU scheduling algorithms:  
**Highest Priority First (HPF)**, **Shortest Remaining Time Next (SRTN)**, and **Round Robin (RR)**  — combined with a **Buddy System Memory Allocation module**. Implemented in C using POSIX message queues for inter-process communication.
---

## Overview

This simulator mimics a real CPU scheduler. It receives processes from a generator and schedules them using one of the three algorithms. Communication between modules is done using **POSIX message queues**, and the system logs performance statistics such as waiting time, turnaround time, and CPU utilization. Buddy System Memory Allocator: Efficient memory management using the buddy allocation strategy, allowing dynamic allocation and deallocation with minimal fragmentation.

---

## Algorithms Implemented

### Highest Priority First (HPF) — Non-preemptive
- Chooses the process with the highest priority (lowest number).
- If multiple have the same priority, earlier arrival wins.
- Once selected, a process runs to completion.

### Shortest Remaining Time Next (SRTN) — Preemptive
- Always runs the process with the least remaining time.
- A newly arriving process with less time can **preempt** the current one.

### Round Robin (RR) — Preemptive
- Uses a **fixed time quantum**.
- Each process runs for one quantum in cyclic order.
- Incomplete processes go to the end of the queue.

## Assumptions

- Processes are sorted by arrival time in the input.
- Priority range: `0` (highest) to `10` (lowest).
- All time units are integers.

---

Each algorithm generates:
- `scheduler.log`: execution timeline
- `memory.log`: logs memory allocations and deallocations
- `scheduler.perf`: performance statistics including:
  - Average Weighted Turnaround Time (WAT)
  - Average Waiting Time (WT)
  - CPU Utilization
  - standard deviation for WTA



