#ifndef FILE_H
#define FILE_H

#include "dc.h"

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

extern KnixFS fs;

void init_fs();
int save_fs();
int load_fs();
uint32 simple_hash(const uint8 *data, size_t size);
int knixfs_write_file(KnixFS_Inode *inode, const uint8 *data, uint32 data_size);
int knixfs_read_file(KnixFS_Inode *inode, uint8 *buffer, uint32 buffer_size);
void free_file_blocks(KnixFS_Inode *inode);

#endif //FILE_H
