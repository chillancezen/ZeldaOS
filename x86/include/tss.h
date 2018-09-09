/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _TSS_H
#define _TSS_H
#include <lib/include/types.h>

struct task_state_segment {
    uint16_t previous_task_selector;
    uint16_t reserved0;
    //privilege level 0 stack
    uint32_t esp0;
    uint16_t ss0;
    uint16_t reserved1;
    //privilege level 1 stack
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved2;
    //privilege level 2 stack
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved3;
    
    uint32_t cr3;//as page directory base register
    uint32_t eip;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp; //stack pointer of current privilege level
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t reserved4;
    uint16_t cs;
    uint16_t reserved5;
    uint16_t ss;
    uint16_t reserved6;
    uint16_t ds;
    uint16_t reserved7;
    uint16_t fs;
    uint16_t reserved8;
    uint16_t gs;
    uint16_t reserved9;

    uint16_t ldt_selector;
    uint16_t reserved10;
    uint16_t reserved11;
    uint16_t iomap;
}__attribute__((packed));

void set_tss_privilege_level0_stack(uint32_t esp0);
void tss_init(void);
#endif
