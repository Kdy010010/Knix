#include "file.h"
#include "type.h"
#include "disk.h"

/*=========================*/
/* 4. 저장되는 파일 시스템 (KnixFS) */
/*=========================*/
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
