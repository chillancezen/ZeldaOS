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
int32_t
spawn_task(struct task * _task)
{

    return 0;
}
#if defined(INLINE_TEST)
int32_t
mockup_load_task(struct task * _task,
    int32_t dpl,
    void (*entry)(void))
{
    _task->privilege_level = dpl;
    _task->entry = (uint32_t)entry;

    _task->privilege_level0_stack =
        malloc_align(DEFAULT_TASK_PRIVILEGED_STACK_SIZE, 4);
    _task->privilege_level3_stack =
        malloc_align(DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE, 4);
    return OK;
}
void
mock_entry(void)
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
    ASSERT(!mockup_load_task(_task, DPL_3, mock_entry));
    ASSERT(!spawn_task(_task));
#endif
}

