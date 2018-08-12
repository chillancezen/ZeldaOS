/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _TASK_H
#define _TASK_H
#include <kernel/include/kernel.h>
#include <lib/include/types.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>


struct task {
    struct task * prev;
    struct task * next;
    /*
     * The x86 cpu state, please refer to x86/include/interrupt.h
     */
    struct x86_cpustate * cpu;

    /*
     * stack at PL 0 and 3, when privilege_level is DPL_0,
     * privilege_level3_stack is NULL.
     */
    void * privilege_level0_stack;
    void * privilege_level3_stack;
    uint32_t entry;
    /*
     * this field specifies in which PL the task runs
     * it often in [DPL_0, DPL_3]
     */
    uint32_t privilege_level:2;
};

#define IS_TASK_KERNEL_TYPE (_task) ((_task)->privilege_level == DPL_0)

#define RUNTIME_STACK(_task) ({\
    void * _stack = NULL; \
    _stack = ((_task)->privilege_level == DPL_0)? \
        (_task)->privilege_level0_stack: \
        (_task)->privilege_level3_stack; \
    ASSERT(_stack); \
    _stack; \
})

void task_init(void);
#endif
