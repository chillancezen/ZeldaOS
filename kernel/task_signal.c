/*
 * Copyright (c) 2018 Jie Zheng
 * 
 * The following diagram shows how signal-context is switched to and from
 * normal execution context
 * 
 *  stack@PL3             stack@PL0
 *    |                     |
 *    |                     |
 *    |  transit(PL3-->PL0) |
 *    | ------------------> |
 *    |                     |
 *    |                     | <----------save(task->cpu)
 *    |                     |
 *    |                     |
 *    |                     | push(task->cpu)
 *    |                         |
 *    |                         | <----save(task->cpu)
 *    |                                               |push(task->cpu)
 *    |                   (Nested Intra-PL0 trap)     |     save(task->cpu)
 *    |                                               |pop(task->cpu)
 *    |                         |
 *    |                     | pop(task->cpu)
 *    |  transit(PL0-->PL3) |  --------+           (Back to normal context)
 *    v <------------------ v          |   <------------------------------+
 *                                     |                                  |
 *sig_stack@PL3         sig_stack@PL0  |                                  |
 *    +                     +          |                                  |
 *    |                     |          v                                  |
 *    |                     |         task->signal_pending                |
 *    |                     |            |         (SIG_ACTION_EXIT)------+
 *    |                     |            |         (SIG_ACTION_STOP)------|
 *    |                     |            |         (SIG_ACTION_IGNORE)----+
 *    |(Frame: return       |  <---------+                                |
 *    | address to PL0 space)       (SIG_ACTION_USER)                     |
 *    |                     |                                             |
 *    |  transit(PL0-->PL3) |                                             |
 *    | <------------------ |                                             |
 *    |                     |                                             |
 *    |  transit(PL3-->PL0) |                                             |
 *    | ------------------> |                                             |
 *    v                     v --------------------------------------------+
 *
 * XXX: signals are only scanned and processed when normal context resumes to
 * be executed into PL3. 
 * XXX: Note that nested user signal handler is not allowed.
 * XXX: and user signal handler is allowed to trap into PL0 context again.
 */
#include <kernel/include/task.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/tss.h>
#include <kernel/include/system_call.h>
#include <kernel/include/zelda_posix.h>

void
task_signal_init(struct task * task)
{
    memset(task->sig_entries, 0x0, sizeof(task->sig_entries));
    
    task->sig_entries[SIGHUP].valid = 1;
    task->sig_entries[SIGHUP].action = SIG_ACTION_EXIT;
    
    task->sig_entries[SIGINT].valid = 1;
    task->sig_entries[SIGINT].overridable = 1;
    task->sig_entries[SIGINT].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGQUIT].valid = 1;
    task->sig_entries[SIGQUIT].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGILL].valid = 1;
    task->sig_entries[SIGILL].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGFPE].valid = 1;
    task->sig_entries[SIGFPE].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGKILL].valid = 1;
    task->sig_entries[SIGKILL].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGSEGV].valid = 1;
    task->sig_entries[SIGSEGV].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGSYS].valid = 1;
    task->sig_entries[SIGSYS].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGPIPE].valid = 1;
    task->sig_entries[SIGPIPE].overridable = 1;
    task->sig_entries[SIGPIPE].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGALARM].valid = 1;
    task->sig_entries[SIGALARM].overridable = 1;
    task->sig_entries[SIGALARM].action = SIG_ACTION_IGNORE;

    task->sig_entries[SIGTERM].valid = 1;
    task->sig_entries[SIGTERM].action = SIG_ACTION_EXIT;

    task->sig_entries[SIGUSR1].valid = 1;
    task->sig_entries[SIGUSR1].overridable = 1;
    task->sig_entries[SIGUSR1].action = SIG_ACTION_IGNORE;

    task->sig_entries[SIGUSR2].valid = 1;
    task->sig_entries[SIGUSR2].overridable = 1;
    task->sig_entries[SIGUSR2].action = SIG_ACTION_IGNORE;

    task->sig_entries[SIGSTOP].valid = 1;
    task->sig_entries[SIGSTOP].action = SIG_ACTION_STOP;

    task->sig_entries[SIGTSTP].valid = 1;
    task->sig_entries[SIGTSTP].action = SIG_ACTION_STOP;

    task->sig_entries[SIGCONT].valid = 1;
    task->sig_entries[SIGCONT].action = SIG_ACTION_CONTINUE;

    task->sig_entries[SIGTTIN].valid = 1;
    task->sig_entries[SIGTTIN].action = SIG_ACTION_STOP;

    task->sig_entries[SIGTTOU].valid = 1;
    task->sig_entries[SIGTTOU].action = SIG_ACTION_STOP;

}


int32_t
signal_task(struct task * task, enum SIGNAL sig)
{
    ASSERT(sig > SIG_INVALID && sig < SIG_MAX);
    if (!task->sig_entries[sig].valid)
        return -ERR_NOT_SUPPORTED;
    LOG_DEBUG("task:0x%x is signalled as %d\n", task, sig);
    {
        // Pre-process signals.
        int processed = 0;
        switch (task->sig_entries[sig].action)
        {
            case SIG_ACTION_STOP:
                if (task->state == TASK_STATE_INTERRUPTIBLE ||
                    task->state == TASK_STATE_RUNNING) {
                    task->non_stop_state = task->state;
                    transit_state(task, TASK_STATE_UNINTERRUPTIBLE);
                }
                processed = 1;
                break;
            case SIG_ACTION_CONTINUE:
                if (task->state == TASK_STATE_UNINTERRUPTIBLE) {
                    transit_state(task, task->non_stop_state);
                    task->non_stop_state = TASK_STATE_ZOMBIE;
                }
                processed = 1;
                break;
        }
        if (processed)
            return OK; 
    }
    task->sig_entries[sig].signaled = 1;
    task->signal_pending = 1;
    raw_task_wake_up(task);
    return OK;
}

uint32_t
return_from_pl3_signal_context(uint32_t * p_esp)
{
    // Paging layout remains unchanged.
    // `current` remains unchanged..
    // ESP will be modified to normal execution cpu state
    LOG_TRIVIA("return from signal context for task:0x%x\n", current);
    ASSERT(current && current->signal_scheduled);
    ASSERT(((uint32_t)current->signaled_cpu) == *p_esp);
    current->signal_scheduled = 0;
    *p_esp = (uint32_t)current->cpu;
    set_tss_privilege_level0_stack(current->privilege_level0_stack_top);
    return OK;
}

static uint32_t
prepare_signal_context(struct task * task, int signal)
{
    uint32_t pl3_stack = USERSPACE_SIGNAL_STACK_TOP - 0x100;
    ASSERT(task->privilege_level == DPL_3);
    ASSERT(signal > SIG_INVALID && signal < SIG_MAX);
    ASSERT(task->sig_entries[signal].valid);
    ASSERT(task->sig_entries[signal].action == SIG_ACTION_USER);
    ASSERT(task->sig_entries[signal].user_entry);
    // Prepare PL3 stack layout
    {
#define PUSH(_stack, _value) { \
        (_stack) -= 4; \
        *(uint32_t *)(_stack) = (uint32_t)(_value); \
}
        ASSERT(!(pl3_stack & 0x3));
        PUSH(pl3_stack, signal);
        PUSH(pl3_stack, (uint32_t)return_from_pl3_signal_context);
#undef PUSH
    }
    // Prepare PL0 stack layout
    {
        struct x86_cpustate * cpu = (struct x86_cpustate *)
            (((uint32_t)task->signaled_privilege_level0_stack_top) -
            sizeof (struct x86_cpustate));
        memset(cpu, 0x0, sizeof(struct x86_cpustate));
        cpu->ss = USER_DATA_SELECTOR;
        cpu->esp = pl3_stack;
        ASSERT(!(cpu->esp & 0x3));
        cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT | EFLAGS_PL3_IOPL;
        cpu->cs = USER_CODE_SELECTOR;
        cpu->eip = task->sig_entries[signal].user_entry;
        cpu->errorcode = 0x0;
        cpu->vector = 0x0;
        cpu->ds = USER_DATA_SELECTOR;
        cpu->es = USER_DATA_SELECTOR;
        cpu->fs = USER_DATA_SELECTOR;
        cpu->gs = USER_DATA_SELECTOR;
        cpu->eax = 0;
        cpu->ecx = 0;
        cpu->edx = 0;
        cpu->ebx = 0;
        cpu->ebp = 0;
        cpu->esi = 0;
        cpu->edi = 0;
        task->signaled_cpu = cpu;
    }
    LOG_TRIVIA("Created signaled context:0x%x for task:0x%x\n",
        task->signaled_cpu, task);
    return OK;
}
uint32_t
task_process_signal(struct x86_cpustate * cpu)
{
#define _(con) if(!(con)) goto out
    int idx = 0;
    int signal = SIG_INVALID;
    uint32_t esp = (uint32_t)cpu;
    _(current);
    _(current->privilege_level == DPL_3);
    _(current->interrupt_depth == 1);
    ASSERT(current->state == TASK_STATE_RUNNING ||
           current->state == TASK_STATE_UNINTERRUPTIBLE);
    if (!current->signal_scheduled && current->signal_pending) {
        // Check with the potential pending signals
        // current->signal_pending will be reset if no signals are found to
        // be pending
        for (idx = SIG_INVALID + 1; idx < SIG_MAX; idx++) {
            if (!current->sig_entries[idx].valid ||
                !current->sig_entries[idx].signaled)
                continue;
            signal = idx;
            break;
        }

        if (signal == SIG_INVALID) {
            current->signal_pending = 0;
        } else {
            ASSERT(current->sig_entries[signal].signaled);
            current->sig_entries[signal].signaled = 0;
            for (idx = signal + 1; idx < SIG_MAX; idx++) {
                if (current->sig_entries[idx].valid &&
                    current->sig_entries[idx].signaled)
                    break;
            }
            if (idx == SIG_MAX)
                current->signal_pending = 0;
            switch(current->sig_entries[signal].action)
            {
                case SIG_ACTION_IGNORE:
                    break;
                case SIG_ACTION_EXIT:
                    transit_state(current, TASK_STATE_EXITING);
                    yield_cpu();
                    break;
                case SIG_ACTION_USER:
                    prepare_signal_context(current, signal);
                    current->signal_scheduled = 1;
                    break;
                default:
                    __not_reach();
                    break;
            }
        }
    }
    out:
        if (current && current->signal_scheduled) {
            esp = (uint32_t)current->signaled_cpu;
            set_tss_privilege_level0_stack(
                current->signaled_privilege_level0_stack_top);
        }
        return esp;
#undef _
}

static int32_t
call_sys_signal(struct x86_cpustate * cpu,
    int32_t signal,
    void (*handler)(int32_t signal))
{
    int32_t ret = -ERR_GENERIC;
    ASSERT(current);
    if (signal <= SIG_INVALID || signal >= SIG_MAX) {
        ret = -ERR_INVALID_ARG;
        goto out;
    }
    if (!current->sig_entries[signal].valid ||
        !current->sig_entries[signal].overridable) {
        ret = -ERR_NOT_SUPPORTED;
        goto out;
    }
    current->sig_entries[signal].action = SIG_ACTION_USER;
    current->sig_entries[signal].user_entry = (uint32_t)handler;
    LOG_DEBUG("Task:0x%x signal:0x%x is overridden to 0x%x\n",
        current, signal, handler);
    ret = OK;
    out:
        return ret;
}
#if defined(INLINE_TEST)
#include <device/include/keyboard.h>
#include <device/include/keyboard_scancode.h>
static struct task * current_pl3_task = NULL;
static int stop = 1;
static void
task_debug_handler(void * arg)
{
    ASSERT(current);
    current_pl3_task = (struct task *)0x800101c;
    signal_task(current_pl3_task, SIGINT);
    //signal_task(current_pl3_task, stop ? SIGSTOP : SIGCONT);
    //signal_task(current_pl3_task, SIGKILL);
    //printk("signal task:0x%x with SIGINT, result:%d\n",
    //    current_pl3_task, signal_task(current_pl3_task, SIGKILL));
    //signal_task(current_pl3_task, stop ? SIGSTOP : SIGCONT));
    stop = !stop;
}


#endif

void
task_signal_sub_init(void)
{
#if defined(INLINE_TEST)
    register_shortcut_entry(SCANCODE_C,
        KEY_STATE_CONTROLL_PRESSED,
        task_debug_handler,
        NULL);
#endif
    register_system_call(SYS_SIGNAL_IDX, 2, (call_ptr)call_sys_signal);
}
