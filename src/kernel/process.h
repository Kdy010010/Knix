//
// Created by user on 2025-03-17.
//

#ifndef PROCESS_H
#define PROCESS_H

#include "dc.h"
#include <stdint.h>

typedef enum { PROC_READY, PROC_RUNNING, PROC_WAITING, PROC_TERMINATED } proc_state_t;

typedef struct {
    int pid;
    proc_state_t state;
    void (*entry_point)();
    uint32_t stack[128];  /* 단순화된 스택 공간 */
    uint32_t sp;          /* 스택 포인터 (인덱스) */
} process_t;

extern process_t process_table[MAX_PROCESSES];
extern int current_process;
extern int next_pid;

void init_processes();
int create_process(void (*entry)());
void schedule();
int sys_create_process(void (*entry)());

#endif //PROCESS_H
