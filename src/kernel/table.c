#include "dc.h"
#include "table.h"
#include "disk.h"
#include "type.h"
#include "file.h"

/*=========================*/
/* 5. File Table & Operations */
/*=========================*/

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
