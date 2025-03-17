#ifndef TABLE_H
#define TABLE_H

#include "dc.h"
#include "file.h"

typedef struct {
    char name[MAX_FILENAME_LEN];
    KnixFS_Inode inode;
    int in_use;
    uint32 mode;
    uint32 owner;
} FileEntry;

extern FileEntry file_table[MAX_FILES];

void init_file_table();
int save_file_table();
int load_file_table();
int find_file_index(const char *name);
int create_file(const char *name, const uint8 *data, uint32 size);
int update_file(const char *name, const uint8 *data, uint32 size);
int delete_file(const char *name);
int copy_file(const char *src, const char *dst);
int rename_file(const char *src, const char *dst);
int append_file(const char *filename, const uint8 *data, uint32 data_size);

#endif //TABLE_H
