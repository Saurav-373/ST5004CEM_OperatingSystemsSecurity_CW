# Task 1 - Process Management and Threading

## Overview

This C program demonstrates process representation, thread creation,
synchronization, race-condition control, deadlock prevention and
round-robin CPU scheduling.

The program has two main parts:

1. A thread simulation using three POSIX worker threads.
2. A round-robin scheduling simulation using process records.

## Features

- Creates three worker threads using pthread_create().
- Waits for all worker threads using pthread_join().
- Protects a shared completed-jobs counter using a mutex.
- Uses a fixed resource-locking order to prevent deadlock.
- Simulates processes using process ID, burst time and remaining time.
- Implements round-robin scheduling with a configurable time quantum.
- Calculates completion time, waiting time and turnaround time.
- Calculates average waiting and turnaround times.
- Rejects invalid process counts, burst times and time quantum values.

## Source File

task1_process_threading.c

## Compilation

```bash
gcc -Wall -Wextra -std=c11 task1_process_threading.c -o task1_simulator -pthread
