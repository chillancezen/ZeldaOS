/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _WAIT_QUEUE_H
#define _WAIT_QUEUE_H
#include <lib/include/types.h>
#include <lib/include/list.h>
#include <kernel/include/task.h>

struct wait_queue_head {
    struct list_elem pivot;
};

struct wait_queue {
    struct task * task;
    struct list_elem list;
};

void
initialize_wait_queue_head(struct wait_queue_head * head);

void
initialize_wait_queue_entry(struct wait_queue * entry, struct task * task);

void
add_wait_queue_entry(struct wait_queue_head * head, struct wait_queue * entry);


void
remove_wait_queue_entry(struct wait_queue_head * head,
    struct wait_queue * entry);

void
wake_up(struct wait_queue_head * head);

#endif
