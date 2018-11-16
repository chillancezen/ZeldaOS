/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _DEV_FS_H
#define _DEV_FS_H
#include <filesystem/include/vfs.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>

void
devfs_init(void);

struct file *
register_dev_node(struct file_system * fs,
    const uint8_t * path,
    uint32_t mode,
    struct file_operation * file_ops,
    void * file_priv);

struct file_system *
get_dev_filesystem(void);

#endif
