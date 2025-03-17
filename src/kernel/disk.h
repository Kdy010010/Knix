#ifndef DISK_H
#define DISK_H

#include "dc.h"

int disk_read(uint32 sector, void *buffer, uint32 count);
int disk_write(uint32 sector, const void *buffer, uint32 count);

#endif //DISK_H
