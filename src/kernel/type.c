#include "type.h"

/*=========================*/
/* 2. Minimal Types & Routines */
/*=========================*/

void *memset(void *dest, int value, size_t count) {
    uint8 *ptr = (uint8*)dest;
    while(count--) { *ptr++ = (uint8)value; }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8 *d = (uint8*)dest;
    const uint8 *s = (const uint8*)src;
    while(count--) { *d++ = *s++; }
    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while(s[len]) { len++; }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while(n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if(n == 0) return 0;
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

uint32 simple_atoi(const char *s) {
    uint32 result = 0;
    while(*s) { result = result * 10 + (*s - '0'); s++; }
    return result;
}

void simple_itoa(uint32 value, char *buf) {
    char temp[16];
    int pos = 0;
    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while(value) { temp[pos++] = '0' + (value % 10); value /= 10; }
    int i;
    for (i = 0; i < pos; i++) { buf[i] = temp[pos - i - 1]; }
    buf[pos] = '\0';
}