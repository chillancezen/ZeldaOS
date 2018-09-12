/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/types.h>
#include <lib/include/generic_tree.h>
#define MAX_PATH 256

struct file {
    uint8_t name[MAX_PATH];
    struct generic_tree fs_node;
};


struct file_operation {
    int32_t (*open)(struct file * _file, uint32_t mode);
    int32_t (*read)(struct file * _file, int offset, void * buffer, int size);
    int32_t (*write)(struct file * _file, int offset, void * buffer, int size);
    int32_t (*truncate)(struct file * _file, int offset);
    int32_t (*close)(struct file * _file);
    int32_t (*ioctl)(struct file * _file, uint32_t request, ...);
};