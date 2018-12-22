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
#include <lib/include/hash_table.h>
#include <kernel/include/userspace_vma.h>
#include <filesystem/include/file.h>
#include <kernel/include/timer.h>
#include <kernel/include/wait_queue.h>
// this default disposition and description of Signals can be found here:
// // https://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/039/3985/3985t1.html
// // In my kernel, only part of them are taken care of.
#include <kernel/include/zelda_posix.h>

enum task_state {
    TASK_STATE_ZOMBIE = 0, // The task state to detect unexpected failure.
    TASK_STATE_RUNNING,
    TASK_STATE_INTERRUPTIBLE,
    TASK_STATE_UNINTERRUPTIBLE,
    TASK_STATE_EXITING,
    TASK_STATE_MAX,
};

struct signal_entry {
    int32_t valid:1;
    int32_t signaled:1;
    int32_t overridable:1;
    int32_t action:5;
#define SIG_ACTION_EXIT 0x0
#define SIG_ACTION_IGNORE 0x1
#define SIG_ACTION_STOP 0x2
#define SIG_ACTION_CONTINUE 0x3
#define SIG_ACTION_USER 0x4
    uint32_t user_entry;
};

struct task {
    // The task_id which identifies the task mainly in userland.
    // the task is stored and searched in the global hash table. the `node`
    // ease the work to store and search
    uint32_t task_id;
    struct hash_node node;
    struct list_elem list;
    // wait queue, when the task exits, it notifies all the tasks which are
    // waiting for termmination of the task
    struct wait_queue_head wq_termination; 
    // `state` is the current state of the task
    // `non_stop_state` is the state of the task before the task goes into 
    // TASK_STATE_UNINTERRUPTIBLE as STOPPED state, we first save the state in
    // it.
    enum task_state state;
    enum task_state non_stop_state;

    // The interrupt depth increases by 1 each time a interrupt(trap) occurs.
    // and decrease by 1 when retruned from trapping(interrupting) context.
    uint32_t interrupt_depth;
    /*
     * The x86 cpu state, please refer to x86/include/interrupt.h
     * including cpu state in signaled context.
     */
    struct x86_cpustate * cpu;
    struct x86_cpustate * signaled_cpu;
    /*
     * per-task VMAs and  page Directory
     */
    struct list_elem vma_list;

    /*
     * If the task is in PL3 context, before switching task, we should 
     * invoke enable_task_paging().
     */
    uint32_t * page_directory;
    /*
     * stack at PL 0 and 3, when privilege_level is DPL_0,
     * privilege_level3_stack is NULL.
     */
    void * privilege_level0_stack;
    void * signaled_privilege_level0_stack;
    /*
     * each time the PL3 code traps into PL0, the priviege level 0 stack
     * top is switched to. 
     * This requires that before the task is scheduled, the 
     * privilege_level0_stack_top is put into TSS(loaded into task register)'s
     * esp0
     */
    uint32_t privilege_level0_stack_top;
    uint32_t signaled_privilege_level0_stack_top;

    // Indicator of signal context.
    // when the signal handler is invoked, it must be set atomically, and
    // cleared after signal handler returns to kernel land.
    uint8_t signal_scheduled;
    uint8_t signal_pending;
    // The Userland/kernel land  entry point.
    uint32_t entry;
    /*
     * this field specifies in which privilege level the task can run
     * it's often in [DPL_0, DPL_3]
     */
    uint32_t privilege_level:2;
    uint32_t exit_code;
    
    // Signal entries
    struct signal_entry sig_entries[SIG_MAX];

    // The file descriptor entries
    struct file_entry file_entries[MAX_FILE_DESCRIPTR_PER_TASK];

    // the name of the task, it could be duplicated
    uint8_t name[MAX_PATH];
    // the current working directory, it can be inherited
    uint8_t cwd[MAX_PATH];

    // XXX:the PL0 per-task private data, the caller which creates the kernel
    // must set it explicitly.
    void * priv;
};

extern struct task * current;
#define IS_TASK_KERNEL_TYPE (_task) ((_task)->privilege_level == DPL_0)

#define disable_preempt() cli()
#define enable_preempt() sti()

#define push_cpu_state(__cpu) {\
    ASSERT(current); \
    (__cpu) = current->cpu; \
}

#define pop_cpu_state(__cpu) {\
    ASSERT(current); \
    current->cpu = (__cpu); \
}

#define push_signal_cpu_state(__cpu) {\
    ASSERT(current); \
    (__cpu) = current->signaled_cpu; \
}

#define pop_signal_cpu_state(__cpu) {\
    ASSERT(current); \
    current->signaled_cpu = (__cpu); \
}

#define signal_pending(_task) (!!(_task)->signal_pending)

struct task *
search_task_by_id(uint32_t id);

int32_t
register_task_in_task_table(struct task * task);

int32_t
unregister_task_from_task_table(struct task * task);

void
task_signal_init(struct task * task);

int32_t
signal_task(struct task * task, enum SIGNAL sig);

void
transit_state(struct task * task, enum task_state target_state);

void
set_work_directory(struct task * task, uint8_t * cwd);

struct list_elem * get_task_list_head(void);
struct task * malloc_task(void);
void free_task(struct task * _task);


uint32_t schedule(struct x86_cpustate * cpu);
void task_init(void);
void task_put(struct task * _task);
struct task * task_get(void);

void schedule_enable(void);
void schedule_disable(void);
int ready_to_schedule(void);

int vma_in_task(struct task * task, struct vm_area * vma);

void dump_task_vm_areas(struct task * _task);
int userspace_map_vm_area(struct task * task, struct vm_area * vma);
int
userspace_map_page(struct task * task,
    uint32_t virt_addr,
    uint32_t phy_addr,
    uint8_t write_permission,
    uint8_t page_writethrough,
    uint8_t page_cachedisable);

int userspace_remap_vm_area(struct task * task, struct vm_area * vma);

int userspace_evict_vma(struct task * task, struct vm_area * vma);
int
userspace_evict_page(struct task * task,
    uint32_t virt_addr,
    int _reclaim_page_table);
int reclaim_page_table(struct task * task, uint32_t virt_addr);

uint32_t reclaim_task(struct task * task);
int enable_task_paging(struct task * task);
void dump_tasks(void);

uint32_t
handle_userspace_page_fault(struct task * task,
    struct x86_cpustate * cpu,
    uint32_t linear_addr);


void
yield_cpu(void);

// XXX: note a kernel task will not be scheduled until you call `task_put` to
// put the task into the running queue
uint32_t
create_kernel_task(void (*entry)(void),
    struct task ** task_ptr,
    uint8_t * task_name);

void
yield_cpu(void);

int32_t
sleep(uint32_t milisecond);

void
task_misc_init(void);

void
task_signal_sub_init(void);

void
raw_task_wake_up(struct task * task);

uint32_t
do_task_traverse(struct taskent * taskp, int32_t count);

#endif
