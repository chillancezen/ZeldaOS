/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _GDT_H
#define _GDT_H

#include <lib/include/types.h>

#define SEG_TYPE_APPLICATION 0x1
#define SEG_TYPE_SYSTEM 0x0

#define SEG_GRANULARITY_BYTE 0x0
#define SEG_GRANULARITY_PAGE 0x1

#define DPL_0 0x0
#define DPL_3 0x3



#define NULL_SELECTOR 0x00
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

#define SELECTOR_INDEX(sel) (((uint32_t)(sel)) >> 3)

struct gdt_entry {
    uint32_t limit_0_15 : 16;
    uint32_t base_0_15 : 16;
    uint32_t base_16_23 : 8;
    uint32_t segment_type : 4;
    uint32_t segmet_class : 1; //0 = SEG_TYPE_SYSTEM, 1 = SEG_TYPE_APPLICATION
    uint32_t dpl : 2;
    uint32_t present : 1;
    uint32_t limit_16_19 : 4;
    uint32_t avail : 1;
    uint32_t long_mode : 1;
    uint32_t operation_size : 1;
    uint32_t granularity : 1;
    uint32_t base_24_31 : 8;
}__attribute__((packed));

struct gdt_pointer {
    uint16_t size;
    uint32_t offset;
}__attribute__((packed));


void gdt_init(void);

#endif

