/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/work_queue.h>
#include <kernel/include/task.h>
#include <kernel/include/zelda_posix.h>
#include <lib/include/string.h>
static void
work_queue_top_level_body(void)
{    
    struct wait_queue wait;
    struct work_queue * work_queue_blob = NULL;
    ASSERT(current);
    ASSERT((work_queue_blob = current->priv));
    initialize_wait_queue_entry(&wait, current);
    add_wait_queue_entry(&work_queue_blob->wq_head, &wait);
    while (1) {
        if (work_queue_blob->to_terminate)
            break;
        transit_state(current, TASK_STATE_INTERRUPTIBLE);
        yield_cpu();
        // check termination flag one more time before
        // doing the bottom half work
        if (work_queue_blob->to_terminate)
            break;
        work_queue_blob->do_work(work_queue_blob->blob);
    }
    signal_task(current, SIGCONT);
    signal_task(current, SIGQUIT);
}

uint32_t
register_work_queue(struct work_queue * work_queue_blob, uint8_t * _name)
{
    uint8_t name[256];
    uint32_t result = OK;
    struct task * wq_task = NULL;
    sprintf((char *)name, "wq:%s", _name);
    result = create_kernel_task(work_queue_top_level_body,
        &wq_task,
        name);
    if (!result) {
        wq_task->priv = work_queue_blob;
        task_put(wq_task);
        LOG_DEBUG("Create work queue :%s as task:0x%x\n", name, wq_task);
    } else {
        LOG_ERROR("Failed to create work queue:%s\n", name);
    }
    return result;
}


uint32_t
notify_work_queue(struct work_queue * work_queue_blob)
{
    if (work_queue_blob->to_terminate) {
        return -ERR_BUSY;
    }
    wake_up(&work_queue_blob->wq_head);
    return OK;
}

uint32_t
terminate_work_queue(struct work_queue * work_queue_blob)
{
    if (work_queue_blob->to_terminate) {
        return -ERR_BUSY;
    }
    ASSERT(!notify_work_queue(work_queue_blob));
    work_queue_blob->to_terminate = 1;
    return OK;
}
