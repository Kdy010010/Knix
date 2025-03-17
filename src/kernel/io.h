#ifndef IO_H
#define IO_H

#include "type.h"

static inline uint8 inb(uint16 port) {
    uint8 result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "dN"(port));
    return result;
}

static inline uint16 inw(uint16 port) {
    uint16 result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "dN"(port));
    return result;
}

static inline void outb(uint16 port, uint8 data) {
    __asm__ volatile ("outb %1, %0" :: "dN"(port), "a"(data));
}

static inline void outw(uint16 port, uint16 data) {
    __asm__ volatile ("outw %1, %0" :: "dN"(port), "a"(data));
}

#endif //IO_H
