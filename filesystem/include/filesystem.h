/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <lib/include/types.h>
#include <kernel/include/zelda_posix.h>

enum FILE_SYSTEM_TYPE {
    ZELDA_FS,
    MEM_FS, /*equivalent to tmpfs in Linux*/
    DEV_FS,
    VSI_FS,
    DUMMY_FS,
};

struct file;
struct filesystem_operation;

struct file_system {
    struct filesystem_operation * fs_ops;    
    uint32_t filesystem_type;

    void * priv;
};

struct filesystem_operation {
    /*
     * Given the file path, search the file in the filesystem-specific
     * directories. return `struct file *` once found.
     */
    struct file * (*fs_open)(struct file_system * fs, const uint8_t * path);
    /*
     * create a file. return the `struct file *` even the file exist.
     */
    struct file * (*fs_create)(struct file_system * fs,
        const uint8_t * path,
        int mode);
    /*
     * create the drectory. return the file entry is successful.
     */
    struct file * (*fs_mkdir)(struct file_system * fs,
        const uint8_t * path);
    /*
     *Delete a file/directory in file_system specfic context
     */
    int32_t (*fs_delete)(struct file_system * fs,
        const uint8_t * path);
};

char * filesystem_type_to_name(int type);

#endif
