/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <lib/include/types.h>

enum FILE_SYSTEM_TYPE {
    ZELDA_FS,
    MEM_FS, /*equivalent to tmpfs in Linux*/
    DEV_FS,
    VSI_FS,
    DUMMY_FS,
};
/*
 * https://github.com/freebsd/freebsd/blob/master/sys/sys/stat.h#L160
 */
struct stat {
    uint16_t st_dev;
    uint16_t st_inode;
    uint32_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint16_t st_rdev;
    uint32_t st_size;
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
     * Given the file path, fill the stat of the file in the `stat`, if no 
     * file is found, OK is returned.
     */
    uint32_t (*fs_stat)(struct file_system * fs,
        const uint8_t * path,
        struct stat * stat);
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
