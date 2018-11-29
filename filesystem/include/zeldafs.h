/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_DRIVE_H
#define _ZELDA_DRIVE_H
#if defined(KERNEL_CONTEXT)
    #include <lib/include/types.h>
    #include <lib/include/list.h>
#else
    #include <stdint.h>
    #error "Not Supported outside of kernel space."
#endif

struct zelda_file {
    uint8_t path[256];
    uint32_t length;
    struct list_elem list;
    uint8_t content[0];
}__attribute__((packed));

void zeldafs_init(void);

struct zelda_file *
search_zelda_file(char * name);

#endif

