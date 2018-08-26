/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/types.h>

enum FILE_SYSTEM_TYPE {
    ZELDA_FS,
    DEV_FS,
    VSI_FS
};

struct file_system {
    
    uint32_t filesystem_type;
};

