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





void
memfs_init(void)
{
    printk("size of:%d %d\n", sizeof(struct mem_block_hdr), BLOCK_AVAIL_SIZE);
}
