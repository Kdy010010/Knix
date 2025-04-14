#ifndef DC_H
#define DC_H

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

typedef unsigned int uint32;
typedef unsigned int uint32_t;
typedef unsigned char uint8;
typedef unsigned long long size_t;
typedef unsigned short uint16;  // ELF 헤더 파싱용

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

#endif //DC_H
