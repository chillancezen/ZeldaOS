/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _TASK_H
#define _TASK_H
#include <kernel/include/kernel.h>
#include <lib/include/types.h>
#include <x86/include/interrupt.h>


struct task {
    struct task * prev;
    struct task * next;
    /*
     * The x86 cpu state, please refer to x86/include/interrupt.h
     */
    struct x86_cpustate * cpu;

    void * privilege_level0_stack;

    uint32_t entry;
    /*
     * this field specifies in which PL the task runs
     * it often in [DPL_0, DPL_3]
     */
    uint32_t privilege_level:2;
};

#define IS_TASK_KERNEL_TYPE (_task) ((_task)->privilege_level == DPL_0)

void task_init(void);
#endif
