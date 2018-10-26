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
    LOG_TRIVIA("task:0x%x is signalled as %d\n", task, sig);
#if defined(PRE_PROCESS_SIGNAL)
    {
        int processed = 0;
        switch (task->sig_entries[sig].action)
        {
            case SIG_ACTION_EXIT:
                transit_state(task, TASK_STATE_EXITING);
                processed = 1;
                break;
            case SIG_ACTION_IGNORE:
                processed = 1;
                break;
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
#endif
    task->sig_entries[sig].signaled = 1;
    task->signal_pending = 1;
    // wake up task if the task is in TASK_STATE_INTERRUPTIBLE
    if (task->state == TASK_STATE_INTERRUPTIBLE)
        transit_state(task, TASK_STATE_RUNNING);
    return OK;
}
