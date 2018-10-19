/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <kernel/include/task.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/tss.h>
#include <memory/include/paging.h>
#include <kernel/include/system_call.h>
#include <kernel/include/zelda_posix.h>

#define CPU_YIELD_TRAP_VECTOR 0x88
static struct list_elem task_list_head;
struct task * current;
static uint32_t __ready_to_schedule;

struct list_elem *
get_task_list_head(void)
{
    return &task_list_head;
}

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
    int terminate = 0;
    uint32_t esp = (uint32_t)cpu;
    struct task * _next_task = NULL;

    /*
     * save current for cpu state
     * and cleanup current task
     */
    if(current) {
        current->cpu = cpu;
        //memcpy(&current->cpu_shadow, cpu, sizeof(struct x86_cpustate));
        task_put(current);
        current = NULL;
    }
    /*
     * pick next task to execute
     */
    select:
    while ((_next_task = task_get())) {
        switch(_next_task->state)
        {
            case TASK_STATE_EXITING:
                LOG_DEBUG("task:0x%x is ready to exit\n", _next_task);
                break;
            case TASK_STATE_RUNNING:
                terminate = 1;
                break;
            case TASK_STATE_ZOMBIE:
                LOG_DEBUG("A zombie task:0x%x\n", _next_task);
                break;
            default:
                __not_reach();
                break;
        }

        if (terminate)
            break;
    }
    if(_next_task) {
        esp = (uint32_t)_next_task->cpu;
        current = _next_task;
        // Actually every time when the contexted switched from PL3 to PL0
        // The SS0:ESP0 is retrieved from current TSS. we can re-use current
        // cpu(esp) to resume selected task, there is no need to calculate
        // a proper cpu(esp) position.
        // memcpy(cpu, &current->cpu_shadow, sizeof(struct x86_cpustate));
        if (_next_task->privilege_level == DPL_3) {
            enable_task_paging(_next_task);
            set_tss_privilege_level0_stack(
                _next_task->privilege_level0_stack_top);
        } else {
            enable_kernel_paging();
        }
    } else {
        // FIXME: Run a long run kernel ide task later which halts the cpu. and
        // run on its dedicated PL0 stack.
        asm volatile("sti;"
            "hlt");
        goto select;
    }
    return esp;
}
/*
 * This is to reclaim the resources which were allocated for the task.
 * free anything include the task itself.
 */
uint32_t
reclaim_task(struct task * task)
{
    LOG_DEBUG("Reclaiming task:0x%x starts\n", task);
    // Detach the task from the global task list
    list_delete(&task_list_head, &task->list);
    // Do not access per-task memory any more
    enable_kernel_paging();
    // Evict all the userspace pages
    {
        struct vm_area * _vma;
        struct list_elem * _list;
        LIST_FOREACH_START(&task->vma_list, _list) {
            _vma = CONTAINER_OF(_list, struct vm_area, list);
            if(!_vma->kernel_vma)
                userspace_evict_vma(task, _vma);
        }
        LIST_FOREACH_END();
        if(task->page_directory) {
            free_base_page((uint32_t)task->page_directory);
        }
    }
    // Free all the vm areas
    {
        struct vm_area * _vma;
        struct list_elem * _list;
        while (!list_empty(&task->vma_list)) {
            _list = list_pop(&task->vma_list);
            ASSERT(_list);
            _vma = CONTAINER_OF(_list, struct vm_area, list);
            free(_vma);
        }
    }
    // Free task's PL0 stack and task itself
    if(task->privilege_level0_stack)
        free(task->privilege_level0_stack);
    free_task(task);
    LOG_DEBUG("Finished reclaiming task::0x%x\n", task);
    return OK;
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
uint32_t
create_kernel_task(void (*entry)(void))
{
    struct x86_cpustate * cpu;
    uint32_t ret = -ERR_GENERIC;
    struct task * task = malloc_task();
    if (!task)
        goto task_error;
    task->privilege_level0_stack =
        malloc_mapped(DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
    if (!task->privilege_level0_stack)
        goto task_error;
    task->privilege_level0_stack_top = (uint32_t)task->privilege_level0_stack +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE;
    task->privilege_level0_stack_top &= ~0x3;
     
    task->privilege_level = DPL_0;
    task->entry = (uint32_t)entry;
    LOG_DEBUG("kernel task:0x%x's PL0 0x%x\n", task,
        task->privilege_level0_stack);
    cpu = (struct x86_cpustate *)(((uint32_t)task->privilege_level0_stack)
        + DEFAULT_TASK_PRIVILEGED_STACK_SIZE 
        - sizeof(struct x86_cpustate));
    cpu = (struct x86_cpustate *)(((uint32_t)cpu) & ~0x3);
    ASSERT(!(((uint32_t)cpu) & 0x3));
    memset(cpu, 0x0, sizeof(struct x86_cpustate));
    cpu->ss = KERNEL_DATA_SELECTOR;
    cpu->esp = (uint32_t)task->privilege_level0_stack +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE;
    task_error:
        if (task) {
            if (task->privilege_level0_stack)
                free(task->privilege_level0_stack);
            free(task);
        }
    return ret;
}
#if defined(INLINE_TEST)
int32_t
mockup_spawn_task(struct task * _task)
{
    struct x86_cpustate * cpu = NULL;
    cpu = (struct x86_cpustate *)
        (_task->privilege_level0_stack +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE -
        sizeof(struct x86_cpustate));
    cpu = (struct x86_cpustate *)(((uint32_t)cpu) & ~0x3f);
    ASSERT(!(((uint32_t)cpu) & 0x3));
    memset(cpu, 0x0, sizeof(struct task));
    if (_task->privilege_level == DPL_3) {
        ASSERT(0);
    } else {
        cpu->ss = KERNEL_DATA_SELECTOR;
        cpu->esp = (uint32_t)_task->privilege_level0_stack +
            DEFAULT_TASK_PRIVILEGED_STACK_SIZE;
        cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT;
        cpu->cs = KERNEL_CODE_SELECTOR;
        cpu->eip = _task->entry;
        cpu->gs = KERNEL_DATA_SELECTOR;
        cpu->fs = KERNEL_DATA_SELECTOR;
        cpu->es = KERNEL_DATA_SELECTOR;
        cpu->ds = KERNEL_DATA_SELECTOR;
    }
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

    LOG_INFO("task-%x's PL0 stack:0x%x - 0x%x\n",
        _task,
        _task->privilege_level0_stack,
        _task->privilege_level0_stack + DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
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

static int32_t
call_sys_exit(struct x86_cpustate * cpu, uint32_t exit_code)
{
    ASSERT(current);
    current->state = TASK_STATE_EXITING;
    yield_cpu();
    return OK;
}
void
yield_cpu(void)
{
    asm volatile("int %0;"
        :
        :"i"(CPU_YIELD_TRAP_VECTOR));
}
static uint32_t
cpu_yield_handler(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;
    if (ready_to_schedule()) {
        esp = schedule(cpu);
    }
    return esp;
}

void
task_init(void)
{
    register_interrupt_handler(CPU_YIELD_TRAP_VECTOR,
        cpu_yield_handler,
        "CPU Yield Trap");
    register_system_call(SYS_EXIT_IDX, 1, (call_ptr)call_sys_exit);
#if !defined(INLINE_TEST)
    struct task * _task = malloc_task();
    ASSERT(_task);
    ASSERT(!mockup_load_task(_task, DPL_0, mockup_entry));
    ASSERT(!mockup_spawn_task(_task));

    _task = malloc_task();
    ASSERT(_task);
    ASSERT(!mockup_load_task(_task, DPL_0, mockup_entry1));
    ASSERT(!mockup_spawn_task(_task));
#endif
    dump_tasks();
}

__attribute__((constructor)) void
task_pre_init(void)
{
    current = NULL;
    __ready_to_schedule = 0;
    list_init(&task_list_head);
}
