/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/types.h>
#include <lib/include/generic_tree.h>

#define FILE_NAME_SIZE 64
struct file {
    uint8_t name[FILE_NAME_SIZE];
    struct generic_tree fs_node;
};


struct file_operation {
    int (*open)(struct file * _file, int32_t mode);
};
