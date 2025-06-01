#include "disk.h"
#include <stdint.h>

/* I/O 포트 접근을 위한 인라인 어셈블리 */
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t data;
    asm volatile ("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void outw(uint16_t port, uint16_t data) {
    asm volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

/* IDE 관련 포트 정의 (Primary IDE 채널, 마스터 디바이스 기준) */
#define ATA_REG_DATA        0x1F0
#define ATA_REG_ERROR       0x1F1  /* 읽기: 오류정보, 쓰기: Features */
#define ATA_REG_SECCOUNT0   0x1F2
#define ATA_REG_LBA0        0x1F3
#define ATA_REG_LBA1        0x1F4
#define ATA_REG_LBA2        0x1F5
#define ATA_REG_HDDEVSEL    0x1F6
#define ATA_REG_STATUS      0x1F7  /* 읽기: 상태, 쓰기: 명령 */

/* ATA 명령 코드 */
#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30

/* 상태 레지스터의 비트 */
#define ATA_SR_BSY   0x80  /* Busy */
#define ATA_SR_DRDY  0x40  /* Device Ready */
#define ATA_SR_DF    0x20  /* Device Fault */
#define ATA_SR_DSC   0x10  /* Device Seek Complete */
#define ATA_SR_DRQ   0x08  /* Data Request */
#define ATA_SR_ERR   0x01  /* Error */

/* 섹터 크기 (바이트) */
#define SECTOR_SIZE 512

/* 간단한 대기 함수: BSY 해제 후 DRQ가 셋될 때까지 대기 */
static int ata_wait_for_drq(void) {
    int timeout = 100000;  // 타임아웃 카운트 (필요시 조정)
    uint8_t status;
    while(timeout--) {
        status = inb(ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                return -1;  /* 에러 발생 */
            }
            if (status & ATA_SR_DRQ) {
                return 0;   /* 읽기/쓰기 준비 완료 */
            }
        }
    }
    return -1;  // 타임아웃
}

/*
 * disk_read
 *  - sector: 읽기를 시작할 논리 섹터 번호 (LBA 방식)
 *  - buffer: 읽은 데이터를 저장할 메모리 버퍼 포인터 (최소 count * 512 바이트 크기)
 *  - count: 읽을 섹터의 수
 *  - 성공 시 0, 실패 시 -1 반환
 */
int disk_read(uint32 sector, void *buffer, uint32 count) {
    uint32 i, j;
    uint16_t *ptr = (uint16_t *)buffer;

    /* LBA 모드로 드라이브 선택, 마스터 디바이스 선택
       0xE0 : 1110 0000, 상위 4비트에 LBA의 27~24비트를 넣음 */
    outb(ATA_REG_HDDEVSEL, 0xE0 | ((sector >> 24) & 0x0F));
    /* 읽을 섹터 수 지정 */
    outb(ATA_REG_SECCOUNT0, (uint8_t)count);
    /* LBA 주소 설정 (하위 24비트) */
    outb(ATA_REG_LBA0, (uint8_t)(sector & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((sector >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((sector >> 16) & 0xFF));
    /* 읽기 명령 전송 */
    outb(ATA_REG_STATUS, ATA_CMD_READ_SECTORS);  // 실제로는 명령 포트에 쓰거나 별도 I/O가 필요
    outb(ATA_REG_STATUS, ATA_CMD_READ_SECTORS);  /* 경우에 따라 두 번 설정 */

    /* 각 섹터마다 읽기 수행 */
    for (i = 0; i < count; i++) {
        if (ata_wait_for_drq() != 0) {
            return -1;  /* 에러나 타임아웃 */
        }
        /* 섹터 당 512바이트 -> 256개의 16비트 워드 읽기 */
        for (j = 0; j < 256; j++) {
            ptr[i * 256 + j] = inw(ATA_REG_DATA);
        }
    }
    return 0;
}

/*
 * disk_write
 *  - sector: 쓰기를 시작할 논리 섹터 번호 (LBA 방식)
 *  - buffer: 기록할 데이터가 저장된 메모리 버퍼 포인터 (최소 count * 512 바이트 크기)
 *  - count: 기록할 섹터의 수
 *  - 성공 시 0, 실패 시 -1 반환
 */
int disk_write(uint32 sector, const void *buffer, uint32 count) {
    uint32 i, j;
    const uint16_t *ptr = (const uint16_t *)buffer;

    /* LBA 모드, 마스터 디바이스 선택 */
    outb(ATA_REG_HDDEVSEL, 0xE0 | ((sector >> 24) & 0x0F));
    /* 쓰기할 섹터 수 지정 */
    outb(ATA_REG_SECCOUNT0, (uint8_t)count);
    /* LBA 주소 설정 */
    outb(ATA_REG_LBA0, (uint8_t)(sector & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((sector >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((sector >> 16) & 0xFF));
    /* 쓰기 명령 전송 */
    outb(ATA_REG_STATUS, ATA_CMD_WRITE_SECTORS);  // 명령 포트에 쓰기

    /* 각 섹터마다 쓰기 수행 */
    for (i = 0; i < count; i++) {
        if (ata_wait_for_drq() != 0) {
            return -1;  /* 에러나 타임아웃 */
        }
        /* 섹터 당 512바이트 -> 256개의 16비트 워드 쓰기 */
        for (j = 0; j < 256; j++) {
            outw(ATA_REG_DATA, ptr[i * 256 + j]);
        }
    }
    return 0;
}
