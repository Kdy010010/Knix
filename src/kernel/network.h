#ifndef NETWORK_H
#define NETWORK_H

#include "dc.h"

/* 포트 입출력 함수 (인라인 어셈블리) */
static inline uint8 inb(uint16 port) {
    uint8 ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16 port, uint8 data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8 inb(uint16 port);  // 이미 선언됨
static inline void outb(uint16 port, uint8 data);  // 이미 선언됨

void ne2k_init();
int ne2k_send(const uint8 *buf, uint16 len);
int ne2k_recv(uint8 *buf, uint16 len);
void network_stack_init();
void network_stack_poll();
void netinfo_cmd();
void nettest_cmd();
void netapp_cmd();

#endif //NETWORK_H
