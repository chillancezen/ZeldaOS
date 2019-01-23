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
#include <filesystem/include/vfs.h>
#include <filesystem/include/zeldafs.h>
#include <kernel/include/elf.h>
/*
 * The task state transition diagram, any exceptional transition is not allowed
 *
 *Running Queue            Blocking Queue                Exiting Queue
 *  |                           |                             +
 *  | <--[TASK:STATE_RUNNING]   |                             |
 *  |                           |                             |
 *  |                           |                             |
 *  |                           |                             |
 *  |                           |                             |
 *  |     (EVENT:SLEEP,WAIT)    |                             |
 *  | ----------------------->  |                             |
 *  | [TASK:STATE_INTERRUPTIBLE]|                             |
 *  |                           |                             |
 *  |    (EVENT:waken up...)    |                             |
 *  |  <----------------------  |                             |
 *  |  [TASK:STATE_RUNNING]     |                             |
 *  |                           |                             |
 *  |     (EVENT:STOP signal)   |                             |
 *  |  -----------------------> |                             |
 *  |  [TASK:STATE_UNINTERRUTIBLE]                            |
 *  |                           |                             |
 *  |                     (EVENT:EXIT signal)                 |
 *+-> ------------------------------------------------------> |
 *| |                     [TASK:STATE_EXITTING]               |
 *| |                           |                             |
 *| |      [EVENT:EXIT signal]  |                             |
 *+---------------------------- |{INTERRUPTIBLE}              |
 *  |     [TASK:STATE_RUNNING]  |                             |
 *  |                           |                             |
 *  |                           |                             |
 *  |      [EVENT:CONT signal]  |                             |
 *  |  <----------------------- |{UNINTERRUPTIBLE}            |
 *  |   [TASK:STATE_RUNNING]    |                             v
 *  |     |                     |
 *  |     +-------------------> |
 *  |     [TASK:STATE_INTERRUPTIBLE]
 *  |                           |
 *  |                           |
 *  |                           |
 *  |                           |
 *  v                           v
 */
static struct list_elem task_list_head;
static struct list_elem task_exit_list_head;
static struct list_elem task_zombie_list_head;
static struct list_elem task_blocking_list_head;
struct task * current;
static uint32_t __ready_to_schedule;
static struct task * kernel_idle_task = NULL;
static enum task_state transition_table[TASK_STATE_MAX][TASK_STATE_MAX];
static struct hash_node kernel_task_hash_heads[KERNEL_TASK_HASH_TABLE_SIZE];
static struct hash_stub kernel_task_hash_stub;
static uint32_t task_seed = 0x0;
static void
kernel_task_hash_table_init(void)
{
    kernel_task_hash_stub.stub_mask = KERNEL_TASK_HASH_TABLE_SIZE -1;
    kernel_task_hash_stub.heads = kernel_task_hash_heads;
    memset(kernel_task_hash_heads, 0x0, sizeof(kernel_task_hash_heads));
}
__attribute__((constructor)) static void
task_state_transition_init(void)
{
#define _(state1, state2) transition_table[(state1)][(state2)] = 1
    memset(transition_table, 0x0, sizeof(transition_table));
    _(TASK_STATE_ZOMBIE, TASK_STATE_ZOMBIE);
    _(TASK_STATE_RUNNING, TASK_STATE_RUNNING);
    _(TASK_STATE_EXITING, TASK_STATE_EXITING);
    _(TASK_STATE_INTERRUPTIBLE, TASK_STATE_INTERRUPTIBLE);
    _(TASK_STATE_UNINTERRUPTIBLE, TASK_STATE_UNINTERRUPTIBLE);
    _(TASK_STATE_RUNNING, TASK_STATE_INTERRUPTIBLE);
    _(TASK_STATE_RUNNING, TASK_STATE_UNINTERRUPTIBLE);
    _(TASK_STATE_RUNNING, TASK_STATE_EXITING);
    _(TASK_STATE_INTERRUPTIBLE, TASK_STATE_RUNNING);
    _(TASK_STATE_INTERRUPTIBLE, TASK_STATE_UNINTERRUPTIBLE);
    _(TASK_STATE_UNINTERRUPTIBLE, TASK_STATE_RUNNING);
    _(TASK_STATE_UNINTERRUPTIBLE, TASK_STATE_INTERRUPTIBLE);
#undef _
}

void
raw_task_wake_up(struct task * task)
{
    switch(task->state)
    {
        case TASK_STATE_RUNNING:
            break;
        case TASK_STATE_INTERRUPTIBLE:
            transit_state(task, TASK_STATE_RUNNING);
            break;
        case TASK_STATE_UNINTERRUPTIBLE:
            // In an `TASK_STATE_UNINTERRUPTIBLE` state, the previous
            // `non_stop_state` must be in `TASK_STATE_INTERRUPTIBLE`, we
            // restore it to RUNNING state, but do not run the task
            // immediately. it muust be continued with explicit SIGCONT signal.
            ASSERT(task->non_stop_state == TASK_STATE_RUNNING ||
                task->non_stop_state == TASK_STATE_INTERRUPTIBLE);
            task->non_stop_state = TASK_STATE_RUNNING;
            break;
        default:
            LOG_ERROR("current task state:%d\n", task->state);
            __not_reach();
            break;
    }
}

static uint8_t *
task_state_to_string(enum task_state stat)
{
    uint8_t * str = (uint8_t *)"UNKNOWN";
    switch(stat)
    {
        case TASK_STATE_ZOMBIE:
            str = (uint8_t *)"TASK_STATE_ZOMBIE";
            break;
        case TASK_STATE_EXITING:
            str = (uint8_t *)"TASK_STATE_EXITING";
            break;
        case TASK_STATE_INTERRUPTIBLE:
            str = (uint8_t *)"TASK_STATE_INTERRUPTIBLE";
            break;
        case TASK_STATE_UNINTERRUPTIBLE:
            str = (uint8_t *)"TASK_STATE_UNINTERRUPTIBLE";
            break;
        case TASK_STATE_RUNNING:
            str = (uint8_t *)"TASK_STATE_RUNNING";
            break;
        default:
            __not_reach();
            break;
    }
    return str;
}
void
transit_state(struct task * task, enum task_state target_state)
{
    enum task_state prev_state;
    prev_state = task->state;
    task->state = target_state;
    ASSERT(target_state < TASK_STATE_MAX);
    ASSERT(transition_table[prev_state][target_state]);
    LOG_TRIVIA("Task:0x%x state transition from:%s to %s\n",
        task,
        task_state_to_string(prev_state),
        task_state_to_string(target_state));
}

void
set_work_directory(struct task * task, uint8_t * cwd)
{
    memset(task->cwd, 0x0, sizeof(task->cwd));
    strcpy_safe(task->cwd, cwd, sizeof(task->cwd));
    LOG_TRIVIA("Task:0x%x cwd changed to:%s\n", task, task->cwd);
}

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
        _task->task_id = task_seed++;
        initialize_wait_queue_head(&_task->wq_termination);
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

static void
process_blocking_task_list(void)
{
    struct list_elem * _list = NULL;
    struct task * _task = NULL;
    LIST_FOREACH_START(&task_blocking_list_head, _list) {
        _task = CONTAINER_OF(_list, struct task, list);
        switch (_task->state)
        {
            case TASK_STATE_RUNNING:
            case TASK_STATE_EXITING:
                list_delete(&task_blocking_list_head, _list);
                task_put(_task);
                break;
            case TASK_STATE_INTERRUPTIBLE:
            case TASK_STATE_UNINTERRUPTIBLE:
                break;
            case TASK_STATE_ZOMBIE:
                list_delete(&task_blocking_list_head, _list);
                list_append(&task_zombie_list_head, _list);
                break;
            default:
                __not_reach();
                break;
        }
    }
    LIST_FOREACH_END();
}
static void
process_exit_task_list(void)
{
    struct list_elem * _list = NULL;
    struct task * _task = NULL;
    while ((_list = list_fetch(&task_exit_list_head))) {
        _task = CONTAINER_OF(_list, struct task, list);
        ASSERT(_task->state == TASK_STATE_EXITING);
        wake_up(&_task->wq_termination);
        reclaim_task(_task);
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

    if(current) {
        if (current != kernel_idle_task)
            task_put(current);
        current = NULL;
    }
    // process other auxiliary tasks queue
    process_blocking_task_list();
    process_exit_task_list();
    // pick next task to execute.
    while ((_next_task = task_get())) {
        switch(_next_task->state)
        {
            case TASK_STATE_EXITING:
                list_append(&task_exit_list_head, &_next_task->list);
                LOG_DEBUG("task:0x%x is ready to exit\n", _next_task);
                break;
            case TASK_STATE_RUNNING:
                terminate = 1;
                break;
            case TASK_STATE_ZOMBIE:
                list_append(&task_zombie_list_head, &_next_task->list);
                LOG_DEBUG("A zombie task:0x%x\n", _next_task);
                break;
            case TASK_STATE_INTERRUPTIBLE:
            case TASK_STATE_UNINTERRUPTIBLE:
                list_append(&task_blocking_list_head, &_next_task->list);
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
    } else {
        // FIXED: Run a long run kernel ide task later which halts the cpu. and
        // run on its dedicated PL0 stack.
        esp = (uint32_t)kernel_idle_task->cpu;
        current = kernel_idle_task;
    }
    ASSERT(current);
    current->schedule_counter++;
    enable_task_paging(current);
    set_tss_privilege_level0_stack(current->privilege_level0_stack_top);
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
    // XXX:The task is not in any queue now. it must be standalone
    ASSERT(!task->list.prev);
    ASSERT(!task->list.next);
    // Do not access per-task memory any more
    // XXX:Do NOT modify current PAGING layout, because task recycling only
    // happens when it's in non-running task queues.
    // enable_kernel_paging();

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
    // close all the file descriptor
    {
        int idx = 0;
        int32_t vfs_result = 0;
        for (idx = 0; idx < MAX_FILE_DESCRIPTR_PER_TASK; idx++) {
            if (!task->file_entries[idx].valid)
                continue;
            ASSERT(task->file_entries[idx].file);
            vfs_result = do_vfs_close(task->file_entries[idx].file);
            task->file_entries[idx].valid = 0;
            task->file_entries[idx].writable = 0;
            task->file_entries[idx].offset = 0;
            task->file_entries[idx].file = NULL;
            LOG_TRIVIA("close remaining open file descriptor: {task:0x%x, "
                "fd:%d, result:%d}\n", task, idx,vfs_result);
        }
    }
    // Free task's PL0 stack and task itself
    if (task->privilege_level0_stack)
        free(task->privilege_level0_stack);
    if (task->signaled_privilege_level0_stack)
        free(task->signaled_privilege_level0_stack);
    ASSERT(unregister_task_from_task_table(task) == OK);
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
        LOG_INFO("task-%d(0x%x) program:%s entry:0x%x\n",
            _task->task_id, _task, _task->name, _task->entry);
    }
    LIST_FOREACH_END();
}
/*
 * This is self-explanatory, it will create a task which runs at PL0.
 * OK is returned if successful and *task_ptr point to the newly created task.
 */
uint32_t
create_kernel_task(void (*entry)(void),
    struct task ** task_ptr,
    uint8_t * task_name)
{
    struct x86_cpustate * cpu;
    uint32_t ret = -ERR_GENERIC;
    struct task * task = malloc_task();
    if (!task)
        goto task_error;
    task->state = TASK_STATE_RUNNING;
    task->privilege_level0_stack =
        malloc_mapped(DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
    if (!task->privilege_level0_stack)
        goto task_error;
    task->privilege_level0_stack_top = (uint32_t)task->privilege_level0_stack +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE;
    task->privilege_level0_stack_top &= ~0x3;
    ASSERT(!(task->privilege_level0_stack_top & 0x3));

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
    // As a matter of fact, the SS:ESP will be ignored in kernel task
    // Intra PL0 task switching involves no stack switching.
    //cpu->ss = KERNEL_DATA_SELECTOR;
    //cpu->esp = task->privilege_level0_stack_top;
    cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT;
    cpu->cs = KERNEL_CODE_SELECTOR;
    cpu->eip = task->entry;
    cpu->gs = KERNEL_DATA_SELECTOR;
    cpu->fs = KERNEL_DATA_SELECTOR;
    cpu->es = KERNEL_DATA_SELECTOR;
    cpu->ds = KERNEL_DATA_SELECTOR;
    task->cpu = cpu;
    task->interrupt_depth = 1;
    strcpy_safe(task->name, task_name, sizeof(task->name));
    set_work_directory(task, (uint8_t *)"/");
    LOG_DEBUG("kernel task 0x%x created\n", task);

    *task_ptr = task;
    ASSERT(OK == register_task_in_task_table(task));
    // XXX:WOW, this place hits a serious bug which caused the kernel panic
    // , the task is recycled and once again the memory is allocated to 
    // other sub-system as new data structure, the task is messed up.
    return OK;
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
void
task_pre_interrupt_handler(struct x86_cpustate * cpu)
{
    if (current) {
        uint32_t old_interrupt_depth = current->interrupt_depth;
        struct x86_cpustate * old_state = NULL;
        current->interrupt_depth++;
        if (current->signal_scheduled) {
            old_state = current->signaled_cpu;
            current->signaled_cpu = cpu;
            LOG_TRIVIA("Save task:0x%x's signaled-cpu:"
                "0x%x-->0x%x [%d --> %d]\n",
                current,old_state,
                cpu, old_interrupt_depth, current->interrupt_depth);
        } else {
            old_state = current->cpu;
            current->cpu = cpu;
            LOG_TRIVIA("Save task:0x%x's normal-cpu:"
                "0x%x-->0x%x [%d --> %d]\n",
                current, old_state, cpu,
                old_interrupt_depth, current->interrupt_depth);
        }
    }
}

void
task_post_interrupt_handler(struct x86_cpustate * cpu)
{
    if (current) {
        uint32_t old_interrupt_depth = current->interrupt_depth;
        struct x86_cpustate * old_state = NULL;
        current->interrupt_depth--;
        ASSERT(((int32_t)current->interrupt_depth) >= 0);
        if (current->signal_scheduled) {
            old_state = current->signaled_cpu;
            LOG_TRIVIA("Restore task:0x%x's signaled-cpu:"
                "0x%x-->0x%x [%d --> %d]\n",
                current,old_state, cpu,
                old_interrupt_depth, current->interrupt_depth);
            //ASSERT(current->signaled_cpu == cpu);
        } else {
            old_state = current->cpu;
            LOG_TRIVIA("Restore task:0x%x's normal-cpu:"
                "0x%x-->0x%x [%d --> %d]\n",
                current, old_state, cpu,
                old_interrupt_depth, current->interrupt_depth);
            //ASSERT(current->cpu == cpu);
        }
    }
}

static uint32_t
task_hash(void * task_id)
{
    return (uint32_t)task_id;
}

static uint32_t
task_identity(struct hash_node * node, void * task_id)
{
    struct task * task = CONTAINER_OF(node, struct task, node);
    return task->task_id == (uint32_t)task_id;
}

struct task *
search_task_by_id(uint32_t id)
{
    struct hash_node * node = NULL;
    node = search_hash_node(&kernel_task_hash_stub,
        (void *)id,
        task_hash,
        task_identity);
    return node ? CONTAINER_OF(node, struct task, node) : NULL;
}
/*
 * rteurn OK if successful. otherwise -ERR_EXIST is returned
 */
int32_t
register_task_in_task_table(struct task * task)
{
    int32_t result = OK;
    result = add_hash_node(&kernel_task_hash_stub,
        (void *)task->task_id,
        &task->node,
        task_hash,
        task_identity);
    return result;
}

int32_t
unregister_task_from_task_table(struct task * task)
{
    int32_t result = OK;
    result = delete_hash_node(&kernel_task_hash_stub,
        (void *)task->task_id,
        task_hash,
        task_identity);
    return result;
}

uint32_t
do_task_traverse(struct taskent * taskp, int32_t count)
{
    int nr_task = 0;
    int idx = 0;
    int32_t to_terminate = 0;
    struct hash_node * _node = NULL;
    struct task * _task = NULL;
    for (idx = 0; idx < KERNEL_TASK_HASH_TABLE_SIZE; idx++) {
        LIST_FOREACH_START(&kernel_task_hash_heads[idx], _node) {
            _task = CONTAINER_OF(_node, struct task, node);
            if (nr_task < count) {
                taskp[nr_task].task_id = _task->task_id;
                taskp[nr_task].state = _task->state;
                taskp[nr_task].non_stop_state = _task->non_stop_state;
                taskp[nr_task].entry = _task->entry;
                taskp[nr_task].privilege_level = _task->privilege_level;
                taskp[nr_task].schedule_counter = _task->schedule_counter;
                strcpy_safe(taskp[nr_task].name,
                    _task->name,
                    sizeof(taskp[nr_task].name));
                strcpy_safe(taskp[nr_task].cwd,
                    _task->cwd,
                    sizeof(taskp[nr_task].cwd));
                nr_task++;
            } else {
                to_terminate = 1;
                break;
            }
        }
        LIST_FOREACH_END();
        if (to_terminate)
            break;
    }
    return nr_task;
}
/*
 * This is the kernel idle task which will always be selected and select 
 */
static void
kernel_idle_task_body(void)
{
    do {
        sti();
        hlt();
    } while (1);
}


void
task_init(void)
{
    task_misc_init();
    task_signal_sub_init();
    ASSERT(OK == create_kernel_task(kernel_idle_task_body,
        &kernel_idle_task,
        (uint8_t *)"kernel_idle_task"));
    ASSERT(kernel_idle_task);
    LOG_INFO("registered kernel idle task:0x%x\n", kernel_idle_task);
    {
        struct zelda_file * zfile = search_zelda_file(USERLAND_INIT_PATH);
        ASSERT(zfile);
        ASSERT(!validate_static_elf32_format(zfile->content, zfile->length));
        ASSERT(!load_static_elf32(zfile->content,
            (uint8_t *)"cwd=\"/\" tty=/dev/console "USERLAND_INIT_PATH"",
            NULL));
    }
    dump_tasks();
}

__attribute__((constructor)) void
task_pre_init(void)
{
    current = NULL;
    __ready_to_schedule = 0;
    list_init(&task_list_head);
    list_init(&task_exit_list_head);
    list_init(&task_zombie_list_head);
    list_init(&task_blocking_list_head);
    kernel_task_hash_table_init();
}
