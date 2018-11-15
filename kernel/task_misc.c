/*
 * Copyright (c) 2018 Jie Zheng
 */


#include <kernel/include/task.h>
#include <kernel/include/system_call.h>
#include <kernel/include/zelda_posix.h>
#include <kernel/include/timer.h>
#include <lib/include/string.h>
#include <filesystem/include/vfs.h>

#define CPU_YIELD_TRAP_VECTOR 0x88
static void
sleep_callback(struct timer_entry * timer, void * priv)
{
    struct task * _task = (struct task *)priv;
    raw_task_wake_up(_task);
}

int32_t
sleep(uint32_t milisecond)
{
    int ret = OK;
    struct timer_entry timer;
    memset(&timer, 0x0, sizeof(timer));
    ASSERT(current);
    timer.state = timer_state_idle;
    timer.time_to_expire = jiffies + milisecond;
    timer.priv = current;
    timer.callback = sleep_callback;
    register_timer(&timer);
    transit_state(current, TASK_STATE_INTERRUPTIBLE);
    yield_cpu();
    if (signal_pending(current)) {
        cancel_timer(&timer);
        ret = -ERR_INTERRUPTED;
    }
    ASSERT(timer_detached(&timer));

    return ret;
}

void
yield_cpu(void)
{
    struct x86_cpustate * cpu;
    struct x86_cpustate * signal_cpu;
    push_cpu_state(cpu);
    push_signal_cpu_state(signal_cpu);
    asm volatile("int %0;"
        :
        :"i"(CPU_YIELD_TRAP_VECTOR));
    pop_signal_cpu_state(signal_cpu);
    pop_cpu_state(cpu);
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

static int32_t
call_sys_exit(struct x86_cpustate * cpu, uint32_t exit_code)
{
    ASSERT(current);
    // FIXED: Signal the task instead of transit the state directly
    current->exit_code = exit_code;
    signal_task(current, SIGCONT);
    signal_task(current, SIGQUIT);
    return OK;
}

static int32_t
call_sys_sleep(struct x86_cpustate * cpu, uint32_t milisecond)
{
    return sleep(milisecond);
}
// If task_id lower than 0. we send signal to `current`

static int32_t
call_sys_kill(struct x86_cpustate * cpu, uint32_t task_id, uint32_t signal)
{
    struct task * task = current;
    if (signal <= SIG_INVALID || signal >= SIG_MAX)
        return -ERR_INVALID_ARG;
    if (((int32_t)task_id) >= 0) {
        task = search_task_by_id(task_id);
    }
    if (!task) {
        return -ERR_NOT_FOUND;
    }
    if (signal == SIGQUIT) {
        signal_task(task, SIGCONT);
    }
    signal_task(task, signal);
    return OK;
}
static int32_t
search_unoccupied_file_descriptor(struct task * task)
{
    int idx = 0;
    int32_t result = -1;
    for (idx = 0; idx < MAX_FILE_DESCRIPTR_PER_TASK; idx++) {
        if (!task->file_entries[idx].valid) {
            result = idx;
            break;
        }
    }
    return result;   
}

static int32_t
call_sys_open(struct x86_cpustate * cpu,
    const uint8_t * path,
    uint32_t flags,
    uint32_t mode)
{
    // FIXME: concatenate current as full path if a relative path is given.
    int32_t fd = -1;
    struct file * file  = NULL;
    ASSERT(current);
    fd = search_unoccupied_file_descriptor(current);
    if (fd < 0) {
        return -ERR_OUT_OF_RESOURCE;
    }
    ASSERT(fd >= 0 && fd < MAX_FILE_DESCRIPTR_PER_TASK);
    ASSERT(!current->file_entries[fd].valid);
    if (flags & O_CREAT) {
        file = do_vfs_create(path, flags, mode);
        if (!file) {
            return -ERR_GENERIC;
        }
    }
    file = do_vfs_open(path, flags, mode);
    if (!file) {
        return -ERR_GENERIC;
    }
    current->file_entries[fd].writable = 0;
    current->file_entries[fd].valid = 1;
    current->file_entries[fd].file = file;
    current->file_entries[fd].offset = 0;
    if ((flags & O_WRONLY) || (flags & O_RDWR)) {
        current->file_entries[fd].writable = 1;
    }
    if (((flags & O_WRONLY) || (flags & O_RDWR)) && (flags & O_TRUNC)) {
        do_vfs_truncate(&current->file_entries[fd], 0x0);
    }
    LOG_TRIVIA("open a file {task:0x%x, path:%s, fd:%d}\n",
        current, path, fd);
    return fd;
}

static int32_t
call_sys_close(struct x86_cpustate * cpu, int32_t fd)
{
    int32_t ret = OK;
    if (fd < 0 || fd >= MAX_FILE_DESCRIPTR_PER_TASK) {
        return -ERR_INVALID_ARG;
    }
    ASSERT(current);
    if (!current->file_entries[fd].valid) {
        return -ERR_INVALID_ARG;
    }
    ret = do_vfs_close(current->file_entries[fd].file);
    current->file_entries[fd].valid = 0;
    current->file_entries[fd].writable = 0;
    current->file_entries[fd].file = NULL;
    current->file_entries[fd].offset = 0;
    LOG_TRIVIA("error closing file {task:0x%x, fd:%d, result:%d}\n",
        current, fd, ret);
    return ret;
}
static int32_t
call_sys_read(struct x86_cpustate * cpu,
    int32_t fd,
    uint8_t * buffer,
    int32_t size_to_read)
{
    int read_result = -ERR_GENERIC;
    ASSERT(current);
    if (fd < 0 ||
        fd >= MAX_FILE_DESCRIPTR_PER_TASK ||
        !current->file_entries[fd].valid) {
        return -ERR_INVALID_ARG;
    }
    read_result = do_vfs_read(&current->file_entries[fd], buffer, size_to_read);
    return read_result;
}

void
task_misc_init(void)
{
    register_interrupt_handler(CPU_YIELD_TRAP_VECTOR,
        cpu_yield_handler,
        "CPU Yield Trap");
    register_system_call(SYS_EXIT_IDX, 1, (call_ptr)call_sys_exit);
    register_system_call(SYS_SLEEP_IDX, 1, (call_ptr)call_sys_sleep);
    register_system_call(SYS_KILL_IDX, 2, (call_ptr)call_sys_kill);
    register_system_call(SYS_OPEN_IDX, 3, (call_ptr)call_sys_open);
    register_system_call(SYS_CLOSE_IDX, 1, (call_ptr)call_sys_close);
    register_system_call(SYS_READ_IDX, 3, (call_ptr)call_sys_read);
}
