/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/vfs.h>
#include <lib/include/string.h>
#include <lib/include/errorcode.h>


static struct mount_entry mount_entries[MOUNT_ENTRY_SIZE];

void
canonicalize_path_name(uint8_t * dst, const uint8_t * src)
{

}
/*
 * Register a file system to the mount point.
 * non-zero returned indicates failure
 */
int
register_system(uint8_t * mount_point, struct file_system * fs)
{
    return OK;
}

void
vfs_init(void)
{
    memset(mount_entries, 0x0, sizeof(mount_entries));
}


