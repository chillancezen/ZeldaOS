/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <filesystem/include/filesystem.h>
#include <filesystem/include/vfs.h>
#include <kernel/include/printk.h>


static struct file *
dummy_fs_open(struct file_system * fs, const uint8_t * path)
{

    return NULL;
}
struct filesystem_operation dummyfs_ops = {
    .fs_open = dummy_fs_open
};
struct file_system dummyfs = {
    .fs_ops = &dummyfs_ops,
    .filesystem_type = DUMMY_FS
};

void
dummyfs_init(void)
{
    ASSERT(!register_file_system((uint8_t *)"/dummy", &dummyfs));
}
