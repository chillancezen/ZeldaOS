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
    int (*open)(struct file * _file, uint32_t mode);
};
