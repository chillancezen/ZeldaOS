/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_DRIVE_H
#define _ZELDA_DRIVE_H
#if defined(KERNEL_CONTEXT)
    #include <lib/include/types.h>
#else
    #include <stdint.h>
#endif

struct zelda_file {
    uint8_t path[256];
    uint32_t length;
};

#endif

