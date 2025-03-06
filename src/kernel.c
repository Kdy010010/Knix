
/*
   Knix OS: 확장형 Unix-like Desktop OS
   - Persistent KnixFS, 파일 테이블, CLI, USB 드라이버
   - 스크립트 실행(exec), 바이너리 실행(execbin: ELF 확장 지원)
   - 간단한 프로세스 관리(멀티태스킹)와 시스템 호출 인터페이스
   - NE2000 기반 간단한 네트워킹 스택 및 네트워킹 명령어(netinfo, nettest, netapp)
*/

/*=========================*/
/* 1. Definitions & Constants */
/*=========================*/
#define BLOCK_SIZE                512
#define MAX_BLOCKS                1024
#define MAX_DIRECT_BLOCKS         10
#define MAX_CMD_LEN               128

/* 디스크 파라미터 */
#define DISK_FS_START_SECTOR      100
#define DISK_FS_SECTOR_COUNT      1032

/* 파일 테이블 파라미터 */
#define MAX_FILENAME_LEN          32
#define MAX_FILES                 16
#define DISK_FILETABLE_START_SECTOR  (DISK_FS_START_SECTOR + DISK_FS_SECTOR_COUNT)
#define DISK_FILETABLE_SECTOR_COUNT  3

/* USB 파라미터 */
#define MAX_USB_DEVICES       4
#define USB_CLASS_HID         0x03
#define USB_SUBCLASS_BOOT     0x01
#define USB_PROTOCOL_KEYBOARD 0x01
#define USB_PROTOCOL_MOUSE    0x02

/* 프로세스 관리 파라미터 */
#define MAX_PROCESSES         4

/* NE2000 NIC 기본 I/O 베이스 (환경에 따라 변경) */
#define NE2K_IO_BASE  0x300

/*=========================*/
/* 2. Minimal Types & Routines */
/*=========================*/
typedef unsigned int  uint32;
typedef unsigned char uint8;
typedef unsigned int  size_t;
typedef unsigned short uint16;  // ELF 헤더 파싱용

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

/*=========================*/
/* 3. Low-Level Disk I/O (Stubs) */
/*=========================*/
int disk_read(uint32 sector, void *buffer, uint32 count) {
    /* 하드웨어 특화 원시 디스크 읽기 구현 필요 */
    (void)sector; (void)buffer; (void)count;
    return 0;
}

int disk_write(uint32 sector, const void *buffer, uint32 count) {
    /* 하드웨어 특화 원시 디스크 쓰기 구현 필요 */
    (void)sector; (void)buffer; (void)count;
    return 0;
}

/*=========================*/
/* 4. Persistent File System (KnixFS) */
/*=========================*/
typedef struct {
    uint8 data[BLOCK_SIZE];
} Block;

typedef struct {
    uint32 size;                      /* 파일 크기 (바이트) */
    uint32 blocks[MAX_DIRECT_BLOCKS]; /* 직접 블록 포인터 */
    uint32 hash;                      /* 파일 내용 해시 */
} KnixFS_Inode;

typedef struct {
    Block blocks[MAX_BLOCKS];
    int free_block_bitmap[MAX_BLOCKS]; /* 1 = free, 0 = allocated */
} KnixFS;

KnixFS fs;

void init_fs() {
    uint32 i;
    memset(&fs, 0, sizeof(KnixFS));
    for (i = 0; i < MAX_BLOCKS; i++) { fs.free_block_bitmap[i] = 1; }
}

int save_fs() {
    uint32 i;
    uint8 *ptr = (uint8*)&fs;
    for (i = 0; i < DISK_FS_SECTOR_COUNT; i++) {
        int ret = disk_write(DISK_FS_START_SECTOR + i, ptr + i * BLOCK_SIZE, 1);
        if (ret != 0) return -1;
    }
    return 0;
}

int load_fs() {
    uint32 i;
    uint8 *ptr = (uint8*)&fs;
    for (i = 0; i < DISK_FS_SECTOR_COUNT; i++) {
        int ret = disk_read(DISK_FS_START_SECTOR + i, ptr + i * BLOCK_SIZE, 1);
        if (ret != 0) return -1;
    }
    return 0;
}

uint32 simple_hash(const uint8 *data, size_t size) {
    uint32 hash = 5381;
    size_t i;
    for (i = 0; i < size; i++) { hash = ((hash << 5) + hash) + data[i]; }
    return hash;
}

int knixfs_write_file(KnixFS_Inode *inode, const uint8 *data, uint32 data_size) {
    if (data_size > BLOCK_SIZE * MAX_DIRECT_BLOCKS) return -1;
    uint32 remaining = data_size, offset = 0;
    uint32 blocks_needed = (data_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32 i, j;
    for (i = 0; i < blocks_needed; i++) {
        int block_index = -1;
        for (j = 0; j < MAX_BLOCKS; j++) {
            if (fs.free_block_bitmap[j]) { block_index = j; fs.free_block_bitmap[j] = 0; break; }
        }
        if (block_index == -1) return -1;
        inode->blocks[i] = block_index;
        uint32 to_copy = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        memcpy(fs.blocks[block_index].data, data + offset, to_copy);
        offset += to_copy;
        remaining -= to_copy;
    }
    inode->size = data_size;
    inode->hash = simple_hash(data, data_size);
    if (save_fs() != 0) return -1;
    return 0;
}

int knixfs_read_file(KnixFS_Inode *inode, uint8 *buffer, uint32 buffer_size) {
    if (buffer_size < inode->size) return -1;
    uint32 remaining = inode->size, offset = 0, i;
    for (i = 0; i < MAX_DIRECT_BLOCKS && remaining > 0; i++) {
        int block_index = inode->blocks[i];
        if (block_index < 0 || block_index >= MAX_BLOCKS) break;
        uint32 to_copy = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        memcpy(buffer + offset, fs.blocks[block_index].data, to_copy);
        offset += to_copy;
        remaining -= to_copy;
    }
    return 0;
}

void free_file_blocks(KnixFS_Inode *inode) {
    uint32 i;
    for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
        int block_index = inode->blocks[i];
        if (block_index >= 0 && block_index < MAX_BLOCKS) {
            fs.free_block_bitmap[block_index] = 1;
            inode->blocks[i] = 0;
        }
    }
    inode->size = 0;
    inode->hash = 0;
    save_fs();
}

/*=========================*/
/* 5. File Table & Operations */
/*=========================*/
typedef struct {
    char name[MAX_FILENAME_LEN];
    KnixFS_Inode inode;
    int in_use;
    uint32 mode;
    uint32 owner;
} FileEntry;

FileEntry file_table[MAX_FILES];

void init_file_table() {
    uint32 i;
    for (i = 0; i < MAX_FILES; i++) {
        file_table[i].in_use = 0;
        file_table[i].mode = 644;
        file_table[i].owner = 0;
    }
}

int save_file_table() {
    uint32 i;
    uint8 *ptr = (uint8*)file_table;
    for (i = 0; i < DISK_FILETABLE_SECTOR_COUNT; i++) {
        int ret = disk_write(DISK_FILETABLE_START_SECTOR + i, ptr + i * BLOCK_SIZE, 1);
        if (ret != 0) return -1;
    }
    return 0;
}

int load_file_table() {
    uint32 i;
    uint8 *ptr = (uint8*)file_table;
    for (i = 0; i < DISK_FILETABLE_SECTOR_COUNT; i++) {
        int ret = disk_read(DISK_FILETABLE_START_SECTOR + i, ptr + i * BLOCK_SIZE, 1);
        if (ret != 0) return -1;
    }
    return 0;
}

int find_file_index(const char *name) {
    uint32 i;
    for (i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use && strcmp(file_table[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}

int create_file(const char *name, const uint8 *data, uint32 size) {
    if (find_file_index(name) != -1) return -1;
    uint32 i, j;
    for (i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].in_use) {
            file_table[i].in_use = 1;
            for (j = 0; j < MAX_FILENAME_LEN - 1 && name[j]; j++)
                file_table[i].name[j] = name[j];
            file_table[i].name[j] = '\0';
            file_table[i].mode = 644;
            file_table[i].owner = 0;
            if (knixfs_write_file(&file_table[i].inode, data, size) != 0) {
                file_table[i].in_use = 0;
                return -1;
            }
            save_file_table();
            return (int)i;
        }
    }
    return -1;
}

int update_file(const char *name, const uint8 *data, uint32 size) {
    int idx = find_file_index(name);
    if (idx == -1) return -1;
    free_file_blocks(&file_table[idx].inode);
    if (knixfs_write_file(&file_table[idx].inode, data, size) != 0)
        return -1;
    save_file_table();
    return 0;
}

int delete_file(const char *name) {
    int idx = find_file_index(name);
    if (idx == -1) return -1;
    free_file_blocks(&file_table[idx].inode);
    file_table[idx].in_use = 0;
    save_file_table();
    return 0;
}

int copy_file(const char *src, const char *dst) {
    int src_idx = find_file_index(src);
    if (src_idx == -1) return -1;
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    if (knixfs_read_file(&file_table[src_idx].inode, buffer, sizeof(buffer)) != 0)
        return -1;
    return create_file(dst, buffer, file_table[src_idx].inode.size);
}

int rename_file(const char *src, const char *dst) {
    int src_idx = find_file_index(src);
    if (src_idx == -1) return -1;
    if (find_file_index(dst) != -1) return -1;
    uint32 j = 0;
    while(j < MAX_FILENAME_LEN - 1 && dst[j]) {
        file_table[src_idx].name[j] = dst[j];
        j++;
    }
    file_table[src_idx].name[j] = '\0';
    save_file_table();
    return 0;
}

int append_file(const char *filename, const uint8 *data, uint32 data_size) {
    int idx = find_file_index(filename);
    if (idx == -1) return -1;
    uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
    uint32 old_size = file_table[idx].inode.size;
    if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0)
        return -1;
    if (old_size + data_size > BLOCK_SIZE * MAX_DIRECT_BLOCKS)
        return -1;
    uint32 i;
    for (i = 0; i < data_size; i++) { buffer[old_size + i] = data[i]; }
    return update_file(filename, buffer, old_size + data_size);
}

/*=========================*/
/* 6. ELF Header (단순화, 32비트) */
/*=========================*/
typedef struct {
    uint8  e_ident[16];
    uint16 e_type;
    uint16 e_machine;
    uint32 e_version;
    uint32 e_entry;
    uint32 e_phoff;
    uint32 e_shoff;
    uint32 e_flags;
    uint16 e_ehsize;
    uint16 e_phentsize;
    uint16 e_phnum;
    uint16 e_shentsize;
    uint16 e_shnum;
    uint16 e_shstrndx;
} Elf32_Ehdr;

/*=========================*/
/* 7. Process Management */
/*=========================*/
typedef enum { PROC_READY, PROC_RUNNING, PROC_WAITING, PROC_TERMINATED } proc_state_t;

typedef struct {
    int pid;
    proc_state_t state;
    void (*entry_point)();
    uint32 stack[128];  /* 단순화된 스택 공간 */
    uint32 sp;          /* 스택 포인터 (인덱스) */
} process_t;

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

/*=========================*/
/* 8. NE2000 NIC & 간단 네트워킹 스택 */
/*=========================*/

/* 포트 입출력 함수 (인라인 어셈블리) */
static inline uint8 inb(uint16 port) {
    uint8 ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16 port, uint8 data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

/* NE2000 레지스터 오프셋 */
#define NE2K_CR       0x00  // Command Register
#define NE2K_PSTART   0x01  // Page Start
#define NE2K_PSTOP    0x02  // Page Stop
#define NE2K_BNRY     0x03  // Boundary Pointer
#define NE2K_TPSR     0x04  // Transmit Page Start
#define NE2K_TBCR0    0x05  // Transmit Byte Count 0
#define NE2K_TBCR1    0x06  // Transmit Byte Count 1
#define NE2K_ISR      0x07  // Interrupt Status

/* NE2000 명령어 플래그 */
#define CR_STP        0x01  // Stop
#define CR_STA        0x02  // Start
#define CR_RD2        0x20  // Remote DMA Command

/* 수신/전송 버퍼 등 기본 파라미터 */
#define NE2K_RXBUF_START  0x40
#define NE2K_RXBUF_STOP   0x80
#define NE2K_TX_PAGE      0x20

void ne2k_init() {
    outb(NE2K_IO_BASE + NE2K_CR, CR_STP);
    outb(NE2K_IO_BASE + NE2K_PSTART, NE2K_RXBUF_START);
    outb(NE2K_IO_BASE + NE2K_PSTOP, NE2K_RXBUF_STOP);
    outb(NE2K_IO_BASE + NE2K_TPSR, NE2K_TX_PAGE);
    outb(NE2K_IO_BASE + NE2K_CR, CR_STA);
}

int ne2k_send(const uint8 *buf, uint16 len) {
    outb(NE2K_IO_BASE + NE2K_TBCR0, len & 0xFF);
    outb(NE2K_IO_BASE + NE2K_TBCR1, (len >> 8) & 0xFF);
    /* 전송 버퍼에 데이터 복사 (실제 구현에서는 DMA 필요) - 생략 */
    outb(NE2K_IO_BASE + NE2K_CR, CR_STA | 0x08); // 0x08: 가상 전송 명령
    return 0;
}

int ne2k_recv(uint8 *buf, uint16 len) {
    uint16 i;
    for (i = 0; i < len; i++) {
        buf[i] = inb(NE2K_IO_BASE + 0x10);  // 가상의 데이터 포트
    }
    return len;
}

void network_stack_init() {
    ne2k_init();
    kprint("NE2000 NIC 초기화 완료.\n");
}

void network_stack_poll() {
    uint8 packet[1500];
    int received = ne2k_recv(packet, sizeof(packet));
    if (received > 0) {
        kprint("패킷 수신됨, 크기: ");
        char numbuf[16];
        simple_itoa(received, numbuf);
        kprint(numbuf);
        kprint(" bytes\n");
        // 이후 Ethernet 프레임 처리 (ARP, IP 등) 확장 가능
    }
}

/* 네트워킹 명령어: netinfo, nettest, netapp */
void netinfo_cmd() {
    kprint("NE2000 NIC 정보:\n");
    kprint("Base I/O: 0x300\n");
    kprint("RX 버퍼: 0x40 ~ 0x80\n");
    kprint("TX 페이지: 0x20\n");
}

void nettest_cmd() {
    uint8 test_packet[64];
    uint16 i;
    for (i = 0; i < 64; i++) {
        test_packet[i] = i;
    }
    ne2k_send(test_packet, 64);
    kprint("테스트 패킷 전송 완료.\n");
}

void netapp_cmd() {
    kprint("네트워킹 응용프로그램 실행 중...\n");
    network_stack_poll();
    kprint("응용프로그램 종료.\n");
}

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

/*=========================*/
/* 10. Networking Commands & Stack */
/*=========================*/
static inline uint8 inb(uint16 port);  // 이미 선언됨
static inline void outb(uint16 port, uint8 data);  // 이미 선언됨

void network_stack_init();
void network_stack_poll();

void netinfo_cmd() {
    kprint("NE2000 NIC 정보:\n");
    kprint("Base I/O: 0x300\n");
    kprint("RX 버퍼: 0x40 ~ 0x80\n");
    kprint("TX 페이지: 0x20\n");
}

void nettest_cmd() {
    uint8 test_packet[64];
    uint16 i;
    for (i = 0; i < 64; i++)
        test_packet[i] = i;
    ne2k_send(test_packet, 64);
    kprint("테스트 패킷 전송 완료.\n");
}

void netapp_cmd() {
    kprint("네트워킹 응용프로그램 실행 중...\n");
    network_stack_poll();
    kprint("응용프로그램 종료.\n");
}

void network_stack_init() {
    ne2k_init();
    kprint("NE2000 NIC 초기화 완료.\n");
}

void network_stack_poll() {
    uint8 packet[1500];
    int received = ne2k_recv(packet, sizeof(packet));
    if (received > 0) {
        kprint("패킷 수신됨, 크기: ");
        char numbuf[16];
        simple_itoa(received, numbuf);
        kprint(numbuf); kprint(" bytes\n");
    }
}

/*=========================*/
/* 11. Process Management & System Calls */
/*=========================*/
/* 이미 위에 정의됨 (init_processes, create_process, schedule, sys_create_process) */

/*=========================*/
/* 12. CLI Command Processing */
/*=========================*/
void process_command(const char *cmd) {
    char tokens[10][MAX_CMD_LEN];
    int token_count = tokenize(cmd, tokens, 10);
    if (token_count == 0) return;
    
    if (strcmp(tokens[0], "help") == 0) {
        kprint("Commands:\n");
        kprint("  help               - 도움말\n");
        kprint("  ls [-l]            - 파일 목록\n");
        kprint("  cat <file>         - 파일 내용 출력\n");
        kprint("  write <file> <msg> - 파일 생성/업데이트\n");
        kprint("  cp <src> <dst>     - 파일 복사\n");
        kprint("  mv <src> <dst>     - 파일 이동/이름 변경\n");
        kprint("  rm <file>          - 파일 삭제\n");
        kprint("  chmod <file> <mode>- 파일 권한 변경\n");
        kprint("  chown <file> <uid> - 파일 소유자 변경\n");
        kprint("  stat <file>        - 파일 정보 출력\n");
        kprint("  touch <file>       - 빈 파일 생성\n");
        kprint("  append <file> <msg>- 파일에 내용 추가\n");
        kprint("  df                 - 남은 디스크 블록 수\n");
        kprint("  usb                - USB 장치 상태\n");
        kprint("  exec <file>        - 스크립트 실행\n");
        kprint("  execbin <file>     - 바이너리 실행 (ELF 확장 지원)\n");
        kprint("  edit <file>        - 텍스트 편집기\n");
        kprint("  find <pattern>     - 파일 검색\n");
        kprint("  sysinfo            - 시스템 정보\n");
        kprint("  fork <bin>         - 바이너리 파일로 프로세스 생성\n");
        kprint("  schedule           - 프로세스 스케줄러 실행\n");
        kprint("  netinfo            - 네트워크 정보 출력\n");
        kprint("  nettest            - 테스트 패킷 전송\n");
        kprint("  netapp             - 네트워킹 응용프로그램 실행\n");
        kprint("  reboot             - 시스템 재부팅\n");
        kprint("  shutdown           - 시스템 종료\n");
        kprint("  exit               - CLI 종료\n");
    }
    else if (strcmp(tokens[0], "ls") == 0) {
        uint32 i;
        if (token_count > 1 && strcmp(tokens[1], "-l") == 0) {
            for (i = 0; i < MAX_FILES; i++) {
                if (file_table[i].in_use) {
                    kprint(file_table[i].name); kprint("\t");
                    char numbuf[16];
                    simple_itoa(file_table[i].inode.size, numbuf); kprint(numbuf); kprint(" bytes\tMode: ");
                    simple_itoa(file_table[i].mode, numbuf); kprint(numbuf); kprint("\tOwner: ");
                    simple_itoa(file_table[i].owner, numbuf); kprint(numbuf); kprint("\n");
                }
            }
        } else {
            for (i = 0; i < MAX_FILES; i++) {
                if (file_table[i].in_use) { kprint(file_table[i].name); kprint("\n"); }
            }
        }
    }
    else if (strcmp(tokens[0], "cat") == 0) {
        if (token_count < 2) { kprint("Usage: cat <filename>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) == 0) {
            uint32 size = file_table[idx].inode.size;
            if (size < sizeof(buffer)) buffer[size] = '\0'; else buffer[sizeof(buffer)-1] = '\0';
            kprint((const char*)buffer); kprint("\n");
        } else { kprint("파일 읽기 오류.\n"); }
    }
    else if (strcmp(tokens[0], "write") == 0) {
        if (token_count < 3) { kprint("Usage: write <filename> <message>\n"); return; }
        char msg[256]; uint32 i, pos = 0;
        for (i = 2; i < (uint32)token_count; i++) {
            uint32 j = 0;
            while(tokens[i][j] && pos < sizeof(msg) - 2) { msg[pos++] = tokens[i][j++]; }
            if (i < (uint32)token_count - 1) { msg[pos++] = ' '; }
        }
        msg[pos] = '\0';
        if (find_file_index(tokens[1]) != -1) {
            if (update_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) == 0)
                kprint("파일 업데이트 성공.\n");
            else
                kprint("파일 업데이트 오류.\n");
        } else {
            if (create_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) != -1)
                kprint("파일 생성 성공.\n");
            else
                kprint("파일 생성 오류.\n");
        }
    }
    else if (strcmp(tokens[0], "cp") == 0) {
        if (token_count < 3) { kprint("Usage: cp <src> <dst>\n"); return; }
        if (copy_file(tokens[1], tokens[2]) != -1)
            kprint("파일 복사 성공.\n");
        else
            kprint("파일 복사 오류.\n");
    }
    else if (strcmp(tokens[0], "mv") == 0) {
        if (token_count < 3) { kprint("Usage: mv <src> <dst>\n"); return; }
        if (rename_file(tokens[1], tokens[2]) == 0)
            kprint("파일 이름 변경 성공.\n");
        else
            kprint("파일 이름 변경 오류.\n");
    }
    else if (strcmp(tokens[0], "rm") == 0) {
        if (token_count < 2) { kprint("Usage: rm <filename>\n"); return; }
        if (delete_file(tokens[1]) == 0)
            kprint("파일 삭제 성공.\n");
        else
            kprint("파일 삭제 오류.\n");
    }
    else if (strcmp(tokens[0], "chmod") == 0) {
        if (token_count < 3) { kprint("Usage: chmod <filename> <mode>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        file_table[idx].mode = simple_atoi(tokens[2]);
        save_file_table();
        kprint("권한 변경 완료.\n");
    }
    else if (strcmp(tokens[0], "chown") == 0) {
        if (token_count < 3) { kprint("Usage: chown <filename> <owner>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        file_table[idx].owner = simple_atoi(tokens[2]);
        save_file_table();
        kprint("소유자 변경 완료.\n");
    }
    else if (strcmp(tokens[0], "stat") == 0) {
        if (token_count < 2) { kprint("Usage: stat <filename>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("파일을 찾을 수 없습니다.\n"); return; }
        kprint("Name: "); kprint(file_table[idx].name); kprint("\nSize: ");
        char numbuf[16];
        simple_itoa(file_table[idx].inode.size, numbuf); kprint(numbuf); kprint(" bytes\nHash: ");
        simple_itoa(file_table[idx].inode.hash, numbuf); kprint(numbuf); kprint("\nMode: ");
        simple_itoa(file_table[idx].mode, numbuf); kprint(numbuf); kprint("\nOwner: ");
        simple_itoa(file_table[idx].owner, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "touch") == 0) {
        if (token_count < 2) { kprint("Usage: touch <filename>\n"); return; }
        if (find_file_index(tokens[1]) == -1) {
            if (create_file(tokens[1], (const uint8*)"", 0) != -1)
                kprint("파일 생성(터치) 성공.\n");
            else
                kprint("터치 오류.\n");
        } else {
            kprint("파일이 이미 존재합니다.\n");
        }
    }
    else if (strcmp(tokens[0], "append") == 0) {
        if (token_count < 3) { kprint("Usage: append <filename> <message>\n"); return; }
        char msg[256]; uint32 i, pos = 0;
        for (i = 2; i < (uint32)token_count; i++) {
            uint32 j = 0;
            while(tokens[i][j] && pos < sizeof(msg)-2) { msg[pos++] = tokens[i][j++]; }
            if (i < (uint32)token_count - 1) { msg[pos++] = ' '; }
        }
        msg[pos] = '\0';
        if (append_file(tokens[1], (const uint8*)msg, (uint32)strlen(msg)) == 0)
            kprint("내용 추가 성공.\n");
        else
            kprint("내용 추가 오류.\n");
    }
    else if (strcmp(tokens[0], "df") == 0) {
        uint32 free_count = 0, i;
        for (i = 0; i < MAX_BLOCKS; i++) {
            if (fs.free_block_bitmap[i]) free_count++;
        }
        kprint("남은 블록 수: ");
        char numbuf[16];
        simple_itoa(free_count, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "usb") == 0) {
        kprint("USB 장치 수: ");
        char numbuf[16];
        simple_itoa(usb_device_count, numbuf); kprint(numbuf); kprint("\n");
    }
    else if (strcmp(tokens[0], "exec") == 0) {
        if (token_count < 2) { kprint("Usage: exec <filename>\n"); return; }
        exec_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "execbin") == 0) {
        if (token_count < 2) { kprint("Usage: execbin <filename>\n"); return; }
        exec_binary_extended(tokens[1]);
    }
    else if (strcmp(tokens[0], "edit") == 0) {
        if (token_count < 2) { kprint("Usage: edit <filename>\n"); return; }
        edit_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "find") == 0) {
        if (token_count < 2) { kprint("Usage: find <pattern>\n"); return; }
        find_file(tokens[1]);
    }
    else if (strcmp(tokens[0], "sysinfo") == 0) {
        sysinfo();
    }
    else if (strcmp(tokens[0], "fork") == 0) {
        if (token_count < 2) { kprint("Usage: fork <binary_file>\n"); return; }
        int idx = find_file_index(tokens[1]);
        if (idx == -1) { kprint("바이너리 파일을 찾을 수 없습니다.\n"); return; }
        uint8 buffer[BLOCK_SIZE * MAX_DIRECT_BLOCKS];
        if (knixfs_read_file(&file_table[idx].inode, buffer, sizeof(buffer)) != 0) {
            kprint("파일 읽기 오류.\n");
            return;
        }
        int pid = sys_create_process((void (*)())buffer);
        if (pid != -1) {
            kprint("새 프로세스 생성, PID: ");
            char numbuf[16];
            simple_itoa(pid, numbuf);
            kprint(numbuf); kprint("\n");
        } else {
            kprint("프로세스 생성 오류.\n");
        }
    }
    else if (strcmp(tokens[0], "schedule") == 0) {
        kprint("프로세스 스케줄러 실행...\n");
        schedule();
        kprint("스케줄러 종료.\n");
    }
    else if (strcmp(tokens[0], "netinfo") == 0) {
        netinfo_cmd();
    }
    else if (strcmp(tokens[0], "nettest") == 0) {
        nettest_cmd();
    }
    else if (strcmp(tokens[0], "netapp") == 0) {
        netapp_cmd();
    }
    else if (strcmp(tokens[0], "reboot") == 0) {
        reboot_system();
    }
    else if (strcmp(tokens[0], "shutdown") == 0) {
        shutdown_system();
    }
    else if (strcmp(tokens[0], "exit") == 0) {
        kprint("CLI 종료...\n");
        while (1);
    }
    else {
        kprint("알 수 없는 명령어입니다. 'help'를 입력하세요.\n");
    }
}

/*=========================*/
/* 13. USB Driver (Stub) */
/*=========================*/
typedef struct {
    uint8 address;
    uint8 device_class;
    uint8 subclass;
    uint8 protocol;
} USB_Device;

USB_Device usb_devices[MAX_USB_DEVICES];
uint32 usb_device_count = 0;

void usb_scan() {
    usb_device_count = 2;
    usb_devices[0].address = 1;
    usb_devices[0].device_class = USB_CLASS_HID;
    usb_devices[0].subclass = USB_SUBCLASS_BOOT;
    usb_devices[0].protocol = USB_PROTOCOL_KEYBOARD;
    
    usb_devices[1].address = 2;
    usb_devices[1].device_class = USB_CLASS_HID;
    usb_devices[1].subclass = USB_SUBCLASS_BOOT;
    usb_devices[1].protocol = USB_PROTOCOL_MOUSE;
}

void usb_keyboard_handler() {
    kprint("USB 키보드 이벤트 발생.\n");
}

void usb_mouse_handler() {
    kprint("USB 마우스 이벤트 발생.\n");
}

void usb_poll() {
    uint32 i;
    for (i = 0; i < usb_device_count; i++) {
        if (usb_devices[i].device_class == USB_CLASS_HID) {
            if (usb_devices[i].protocol == USB_PROTOCOL_KEYBOARD)
                usb_keyboard_handler();
            else if (usb_devices[i].protocol == USB_PROTOCOL_MOUSE)
                usb_mouse_handler();
        }
    }
}

/*=========================*/
/* 14. Kernel Main */
/*=========================*/
void kmain() {
    int ret;
    char cmdline[MAX_CMD_LEN];
    
    ret = load_fs();
    if (ret != 0) {
        init_fs();
        ret = save_fs();
        if (ret != 0) while(1);
        kprint("새 FS 초기화 완료.\n");
    } else {
        kprint("기존 FS 로드 완료.\n");
    }
    
    ret = load_file_table();
    if (ret != 0) {
        init_file_table();
        ret = save_file_table();
        if (ret != 0) while(1);
        kprint("새 파일 테이블 초기화 완료.\n");
    } else {
        kprint("기존 파일 테이블 로드 완료.\n");
    }
    
    init_processes();
    
    usb_scan();
    kprint("USB 장치 스캔 완료.\n");
    usb_poll();
    
    network_stack_init();
    
    /* 메인 CLI 루프와 함께 주기적으로 네트워킹 패킷 폴링 */
    while (1) {
        kprint("knix> ");
        kgets(cmdline, MAX_CMD_LEN);
        process_command(cmdline);
        network_stack_poll();
    }
    
    while(1);
}
