# Task 2: Memory Management Simulation

## Overview

This program simulates virtual memory paging using two page replacement algorithms:

- First In, First Out (FIFO)
- Least Recently Used (LRU)

The user enters the page size, number of memory frames, and a sequence of logical addresses. The program converts each logical address into a page number before running both algorithms.

## Features

- Configurable page size
- Configurable number of memory frames
- Logical address to page number conversion
- FIFO page replacement
- LRU page replacement
- Page hit and page fault tracking
- Hit ratio and miss ratio calculation
- Frame-by-frame output
- Input validation for invalid values

## Compilation

```bash
gcc -Wall -Wextra -std=c11 task2_memory_management.c -o memory_simulator
