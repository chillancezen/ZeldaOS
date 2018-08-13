/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <kernel/include/task.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>

static struct list_elem task_list_head;
struct task * current;
static uint32_t __ready_to_schedule;
int
ready_to_schedule(void)
{
   return !!__ready_to_schedule; 
}

void
schedule_enable(void)
{
    __ready_to_schedule = 1;
}

void
schedule_disable(void)
{
    __ready_to_schedule = 0;
}

void
task_put(struct task * _task)
{
    list_append(&task_list_head, &_task->list);
}
struct task *
task_get(void)
{
    struct list_elem * _elem = list_fetch(&task_list_head);
    if(!_elem)
        return NULL;
    return CONTAINER_OF(_elem, struct task, list);
}
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
/*
 * The function is to pick up a task in the list,
 * and the task is about to execute on the CPU
 * if no task is selected, the 0 is returned
 */
uint32_t
schedule(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;
    struct task * _next_task = NULL;
    /*
     * save current for cpu state
     * and cleanup current task
     */
    if(current) {
        current->cpu = cpu;
        task_put(current);
        current = NULL;
    }
    /*
     * pick next task to execute
     */
    _next_task = task_get();
    if(_next_task) {
        esp = (uint32_t)_next_task->cpu;
        current = _next_task;
    }
    return esp;
}

void
dump_tasks(void)
{
    struct list_elem * _elem;
    struct task * _task;
    LOG_INFO("Dump tasks:\n");
    LIST_FOREACH_START(&task_list_head, _elem) {
        _task = CONTAINER_OF(_elem, struct task, list);
        LOG_INFO("task-%x entry:0x%x\n", _task, _task->entry);
    }
    LIST_FOREACH_END();
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
    memset(cpu, 0x0, sizeof(struct task));
    cpu->ss = KERNEL_DATA_SELECTOR;
    cpu->esp = _task->privilege_level0_stack_top;
    cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT;
    cpu->cs = KERNEL_CODE_SELECTOR;
    cpu->eip = _task->entry;
    cpu->gs = KERNEL_DATA_SELECTOR;
    cpu->fs = KERNEL_DATA_SELECTOR;
    cpu->es = KERNEL_DATA_SELECTOR;
    cpu->ds = KERNEL_DATA_SELECTOR;
    _task->cpu = cpu;
    task_put(_task);
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
    _task->privilege_level0_stack_top &= ~0xff;
    _task->privilege_level3_stack_top &= ~0xff;

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
    while(1) {
        cli();
        printk("task:%s\n",__FUNCTION__);
        sti();
        asm ("hlt");
    }
}

void
mockup_entry1(void)
{
    while(1) {
        cli();
        printk("task:%s\n", __FUNCTION__);
        sti();
        asm ("hlt");
    }
}
#endif
void
task_init(void)
{
    current = NULL;
    __ready_to_schedule = 0;
    list_init(&task_list_head);
#if defined(INLINE_TEST)
    struct task * _task = malloc_task();
    ASSERT(_task);
    ASSERT(!mockup_load_task(_task, DPL_0, mockup_entry));
    ASSERT(!mockup_spawn_task(_task));

    _task = malloc_task();
    ASSERT(_task);
    ASSERT(!mockup_load_task(_task, DPL_3, mockup_entry1));
    ASSERT(!mockup_spawn_task(_task));
#endif
    dump_tasks();
}

