#include "disk.h"

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