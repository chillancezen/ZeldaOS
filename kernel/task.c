/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <kernel/include/task.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
/*
 * allocate a task structure, return NULL upon memory outage
 */
struct task *
malloc_task(void)
{
    struct task * _task = NULL;
    _task = malloc(sizeof(struct task));
    if (_task) {
        memset(_task, 0x0, sizeof(struct task));
    }
    return _task;
}

void
free_task(struct task * _task)
{
    if (_task) {
        if(_task->privilege_level0_stack)
            free(_task->privilege_level0_stack);
        if(_task->privilege_level3_stack)
            free(_task->privilege_level3_stack);
        free(_task);
    }
}
#if defined(INLINE_TEST)
int32_t
mockup_spawn_task(struct task * _task)
{
    struct x86_cpustate * cpu = NULL;
    uint32_t runtime_stack = (uint32_t)RUNTIME_STACK(_task);
    LOG_INFO("task-%x's runtime stack:%x\n",_task, runtime_stack);
    cpu = (struct x86_cpustate *)(runtime_stack - sizeof(struct x86_cpustate));
    ASSERT(!(((uint32_t)cpu) & 0x3));
    cpu->ss = KERNEL_DATA_SELECTOR;
    cpu->esp = _task->privilege_level0_stack_top;
    cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT;
    cpu->cs = KERNEL_CODE_SELECTOR;
    cpu->eip = _task->entry;
    return 0;
}

int32_t
mockup_load_task(struct task * _task,
    int32_t dpl,
    void (*entry)(void))
{
    _task->privilege_level = dpl;
    _task->entry = (uint32_t)entry;

    _task->privilege_level0_stack =
        malloc(DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
    _task->privilege_level3_stack =
        malloc(DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE);
    _task->privilege_level0_stack_top =
        (uint32_t)_task->privilege_level0_stack +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE;
    _task->privilege_level3_stack_top =
        (uint32_t)_task->privilege_level3_stack +
        DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE;
    _task->privilege_level0_stack_top &= ~0x3;
    _task->privilege_level3_stack_top &= ~0x3;
    LOG_INFO("task-%x's PL0 stack:0x%x - 0x%x\n",
        _task,
        _task->privilege_level0_stack,
        _task->privilege_level0_stack_top);
    LOG_INFO("task-%x's PL3 stack:0x%x - 0x%x\n",
        _task,
        _task->privilege_level3_stack,
        _task->privilege_level3_stack_top);
    return OK;
}
void
mockup_entry(void)
{
    while(1)
        printk("task\n");
}
#endif
void
task_init(void)
{
#if defined(INLINE_TEST)
    struct task * _task = malloc_task();
    ASSERT(_task);
    ASSERT(!mockup_load_task(_task, DPL_0, mockup_entry));
    ASSERT(!mockup_spawn_task(_task));
#endif
}

