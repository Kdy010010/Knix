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
    while (*str) {
        __asm__ volatile (
            "movb %0, %%al\n"
            "outb %%al, $0x3F8\n"
            :
            : "r" (*str)
        );
        str++;
    }
}

void kprint_hex(uint32 num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];  // 8자리 + NULL
    int i;

    buffer[8] = '\0';  // 문자열 끝

    for (i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];  // 마지막 4비트(16진수) 추출
        num >>= 4;  // 다음 4비트 처리
    }

    kprint("0x");
    kprint(buffer);
}
int kgetchar() {
    unsigned char status;
    unsigned char scancode;

    // Wait until the output buffer is full (bit 0 of 0x64 is set)
    do {
        __asm__ volatile ("inb $0x64, %0" : "=a"(status));
    } while ((status & 0x01) == 0);

    // Read from keyboard data port
    __asm__ volatile ("inb $0x60, %0" : "=a"(scancode));

    // Simple US QWERTY keymap for demonstration (only lowercase letters and digits)
    static char scancode_table[128] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', /* 0x00-0x0E */
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',     /* 0x0F-0x1C */
        0,   'a','s','d','f','g','h','j','k','l',';','\'','`',         /* 0x1D-0x29 */
        0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,           /* 0x2A-0x35 */
        '*', 0,  ' ', /* rest ignored */
    };

    // Handle only key-press (not key-release: high bit set)
    if (scancode & 0x80) {
        return 0; // key release, ignore
    } else {
        return scancode_table[scancode];
    }
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
    if (idx == -1) { kprint("File not found.\n"); return; }
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
       kprint("File read error.\n");
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
    if (idx == -1) { kprint("The binary file could not be found.\n"); return; }
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
         kprint("Binary file read error.\n");
         return;
    }
    if (buffer[0] == 0x7F && buffer[1] == 'E' &&
        buffer[2] == 'L' && buffer[3] == 'F') {
         kprint("ELF binary detected.\n");
         Elf32_Ehdr *header = (Elf32_Ehdr*)buffer;
         char numbuf[16];
         kprint("Entry Point: ");
         simple_itoa(header->e_entry, numbuf);
         kprint(numbuf); kprint("\n");
         void (*entry_point)() = (void (*)())(buffer + (header->e_entry));
         entry_point();
    } else {
         kprint("Running flat binary...\n");
         void (*entry_point)() = (void (*)())buffer;
         entry_point();
    }
}

void edit_file(const char *filename) {
    char buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    int idx = find_file_index(filename);
    if (idx != -1) {
        if (knixfs_read_file(&file_table[idx].inode, (uint8*)buffer, sizeof(buffer)) != 0) {
            kprint("File read error.\n");
            return;
        }
    } else {
        buffer[0] = '\0';
    }
    kprint("Current File Content:\n");
    kprint(buffer);
    kprint("\nPlease enter new content (one line):\n");
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
        kprint("File saved.\n");
    } else {
        kprint("File content is too long.\n");
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
    if (!found) kprint("No matching file exists.\n");
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