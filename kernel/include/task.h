/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _TASK_H
#define _TASK_H
#include <kernel/include/kernel.h>
#include <lib/include/types.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>
#include <lib/include/list.h>
#include <kernel/include/userspace_vma.h>


struct task {
    struct list_elem list;
    /*
     * The x86 cpu state, please refer to x86/include/interrupt.h
     */
    struct x86_cpustate * cpu;
    /*
     * per-task VMAs and  page Directory
     */
    struct list_elem vma_list;

    uint32_t page_directory;
    /*
     * stack at PL 0 and 3, when privilege_level is DPL_0,
     * privilege_level3_stack is NULL.
     */
    void * privilege_level0_stack;
    void * privilege_level3_stack;
    uint32_t privilege_level0_stack_top;
    uint32_t privilege_level3_stack_top;
    uint32_t entry;
    /*
     * this field specifies in which PL the task runs
     * it often in [DPL_0, DPL_3]
     */
    uint32_t privilege_level:2;
};
extern struct task * current;
#define IS_TASK_KERNEL_TYPE (_task) ((_task)->privilege_level == DPL_0)

#define RUNTIME_STACK(_task) ({\
    uint32_t _stack = 0; \
    _stack = ((_task)->privilege_level == DPL_0)? \
        (_task)->privilege_level0_stack_top: \
        (_task)->privilege_level3_stack_top; \
    ASSERT(_stack); \
    _stack; \
})
struct task * malloc_task(void);
void free_task(struct task * _task);


uint32_t schedule(struct x86_cpustate * cpu);
void task_init(void);
void task_put(struct task * _task);
struct task * task_get(void);

void schedule_enable(void);
void schedule_disable(void);
int ready_to_schedule(void);
#endif
