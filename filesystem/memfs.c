/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/memfs.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>
#include <filesystem/include/vfs.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>

struct filesystem_operation tmpfs_ops = {
    .fs_open = NULL
};

static struct file_system tmpfs_home = {
    .filesystem_type = MEM_FS,
    .fs_ops = &tmpfs_ops
};

static struct file_system tmpfs_tmp = {
    .filesystem_type = MEM_FS,
    .fs_ops = &tmpfs_ops
};

void
memfs_init(void)
{
   ASSERT(!register_file_system((uint8_t *)"/home/", &tmpfs_home));
   ASSERT(!register_file_system((uint8_t *)"/tmp/", &tmpfs_tmp));
}
