/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <kernel/include/task.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
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
        _task->privilege_level0_stack =
            malloc_align(DEFAULT_TASK_PRIVILEGE_STACK_SIZE, 0x4);
        if(!_task->privilege_level0_stack) {
            goto error;
        }
    }
    return _task;
    error:
        if(_task)
            free(_task);
        return NULL;
}
void
free_task(struct task * _task)
{
    if (_task) {
        if(_task->privilege_level0_stack) {
            free(_task->privilege_level0_stack);
        }
        free(_task);
    }
}

void
task_init(void)
{
    struct task * _task = malloc_task();
    struct task * _task1 = malloc_task();
    printk("task %x\n", _task1);
    ASSERT(_task);
    dump_recycle_bins();
    free_task(_task);
    dump_recycle_bins();
}

