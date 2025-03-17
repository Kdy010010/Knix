#ifndef TYPE_H
#define TYPE_H

#include "dc.h"

void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
uint32 simple_atoi(const char *s);
void simple_itoa(uint32 value, char *buf);

#endif //TYPE_H
