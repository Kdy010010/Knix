#ifndef KPRINT_H
#define KPRINT_H

#include "dc.h"

void kprint(const char *str);
void kprint_hex(uint32 num);
int kgetchar();
void kgets(char *buffer, size_t maxlen);
int tokenize(const char *cmd, char tokens[][MAX_CMD_LEN], int max_tokens);
void exec_file(const char *filename);
void exec_binary_extended(const char *filename);
void edit_file(const char *filename);
void find_file(const char *pattern);
char *strstr(const char *haystack, const char *needle);

#endif //KPRINT_H
