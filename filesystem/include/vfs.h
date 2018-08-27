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
    uint32_t valid:1;
};

void vfs_init(void);
void dump_mount_entries(void);
int canonicalize_path_name(uint8_t * dst, const uint8_t * src);
int register_file_system(uint8_t * mount_point, struct file_system * fs);

#endif
