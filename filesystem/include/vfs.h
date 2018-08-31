/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _VFS_H
#define _VFS_H
#include <lib/include/types.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>

#define MOUNT_ENTRY_SIZE 256 // The maximum entries is 256

struct mount_entry {
    uint8_t mount_point[MAX_PATH];
    struct file_system * fs; 
};

void vfs_init(void);

#endif
