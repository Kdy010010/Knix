#include "kprint.h"
#include "file.h"
#include "table.h"
#include "process.h"
#include "command.h"
#include "type.h"

/*=========================*/
/* 9. CLI, 스크립트, 바이너리 실행, 텍스트 편집, 파일 검색 */
/*=========================*/

void kprint(const char *str) {
    /* Stub: 실제 환경에 맞게 출력 구현 (예: 비디오 메모리, 시리얼 포트) */
    (void)str;
}

int kgetchar() {
    /* Stub: 키보드 입력 반환 */
    return 0;
}

void kgets(char *buffer, size_t maxlen) {
    size_t i = 0;
    int ch;
    while(i < maxlen - 1) {
        ch = kgetchar();
        if (ch == '\n' || ch == '\r') break;
        buffer[i++] = (char)ch;
    }
    buffer[i] = '\0';
}

int tokenize(const char *cmd, char tokens[][MAX_CMD_LEN], int max_tokens) {
    int token_count = 0, i = 0, j = 0;
    char c;
    int in_token = 0;
    while ((c = cmd[i]) != '\0' && token_count < max_tokens) {
        if (c == ' ') {
            if (in_token) {
                tokens[token_count][j] = '\0';
                token_count++;
                j = 0;
                in_token = 0;
            }
        } else {
            tokens[token_count][j++] = c;
            in_token = 1;
        }
        i++;
    }
    if (in_token && token_count < max_tokens) {
        tokens[token_count][j] = '\0';
        token_count++;
    }
    return token_count;
}

void exec_file(const char *filename) {
    int idx = find_file_index(filename);
    if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
       kprint("파일 읽기 오류.\n");
       return;
    }
    char *ptr = (char*)buffer;
    while (*ptr != '\0') {
        char command[MAX_CMD_LEN];
        uint32 i = 0;
        while (*ptr != '\n' && *ptr != '\0' && i < MAX_CMD_LEN - 1)
            command[i++] = *ptr++;
        command[i] = '\0';
        if (command[0] != '\0') process_command(command);
        if (*ptr == '\n') ptr++;
    }
}

void exec_binary_extended(const char *filename) {
    int idx = find_file_index(filename);
    if (idx == -1) { kprint("바이너리 파일을 찾을 수 없습니다.\n"); return; }
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
         kprint("바이너리 파일 읽기 오류.\n");
         return;
    }
    if (buffer[0] == 0x7F && buffer[1] == 'E' &&
        buffer[2] == 'L' && buffer[3] == 'F') {
         kprint("ELF 바이너리 감지됨.\n");
         Elf32_Ehdr *header = (Elf32_Ehdr*)buffer;
         char numbuf[16];
         kprint("엔트리 포인트: ");
         simple_itoa(header->e_entry, numbuf);
         kprint(numbuf); kprint("\n");
         void (*entry_point)() = (void (*)())(buffer + (header->e_entry));
         entry_point();
    } else {
         kprint("플랫 바이너리 실행 중...\n");
         void (*entry_point)() = (void (*)())buffer;
         entry_point();
    }
}

void edit_file(const char *filename) {
    char buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    int idx = find_file_index(filename);
    if (idx != -1) {
        if (knixfs_read_file(&file_table[idx].inode, (uint8*)buffer, sizeof(buffer)) != 0) {
            kprint("파일 읽기 오류.\n");
            return;
        }
    } else {
        buffer[0] = '\0';
    }
    kprint("현재 파일 내용:\n");
    kprint(buffer);
    kprint("\n새로운 내용을 입력하세요 (한 줄):\n");
    char new_line[MAX_CMD_LEN];
    kgets(new_line, MAX_CMD_LEN);
    uint32 current_len = strlen(buffer);
    if (current_len + strlen(new_line) + 1 < sizeof(buffer)) {
        if (current_len > 0) buffer[current_len++] = '\n';
        uint32 k = 0;
        while(new_line[k] && current_len < sizeof(buffer) - 1)
            buffer[current_len++] = new_line[k++];
        buffer[current_len] = '\0';
        if (idx != -1)
            update_file(filename, (const uint8*)buffer, current_len);
        else
            create_file(filename, (const uint8*)buffer, current_len);
        kprint("파일 저장됨.\n");
    } else {
        kprint("파일 내용이 너무 깁니다.\n");
    }
}

void find_file(const char *pattern) {
    uint32 i;
    int found = 0;
    for (i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use) {
            if (strstr(file_table[i].name, pattern) != 0) {
                kprint(file_table[i].name); kprint("\n");
                found = 1;
            }
        }
    }
    if (!found) kprint("일치하는 파일이 없습니다.\n");
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && (*h == *n)) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}