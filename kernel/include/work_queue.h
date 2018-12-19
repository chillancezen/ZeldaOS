/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _WORK_QUEUE_H
#define _WORK_QUEUE_H
#include <kernel/include/task.h>
#include <kernel/include/wait_queue.h>


struct work_queue {
    uint32_t to_terminate;
    struct wait_queue_head wq_head;
    void * blob;
    void (*do_work)(void * blob);
};


uint32_t
register_work_queue(struct work_queue * work_queue_blob, uint8_t * name);

uint32_t
notify_work_queue(struct work_queue * work_queue_blob);

uint32_t
terminate_work_queue(struct work_queue * work_queue_blob);

#endif
