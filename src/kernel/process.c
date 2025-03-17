#include "process.h"

/*=========================*/
/* 7. Process Management */
/*=========================*/

process_t process_table[MAX_PROCESSES];
int current_process = 0;
int next_pid = 1;

void init_processes() {
    uint32 i;
    for (i = 0; i < MAX_PROCESSES; i++)
        process_table[i].state = PROC_TERMINATED;
}

int create_process(void (*entry)()) {
    uint32 i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_TERMINATED) {
            process_table[i].pid = next_pid++;
            process_table[i].state = PROC_READY;
            process_table[i].entry_point = entry;
            process_table[i].sp = 127;  /* 스택 최상위 인덱스 */
            return process_table[i].pid;
        }
    }
    return -1;
}

/* 단순 라운드 로빈 스케줄러 */
void schedule() {
    while(1) {
        int i;
        for (i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].state == PROC_READY) {
                process_table[i].state = PROC_RUNNING;
                process_table[i].entry_point();
                process_table[i].state = PROC_TERMINATED;
            }
        }
        int all_terminated = 1;
        for (i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].state != PROC_TERMINATED) { all_terminated = 0; break; }
        }
        if (all_terminated) break;
    }
}

/* 시스템 호출 예제 */
int sys_create_process(void (*entry)()) {
    return create_process(entry);
}