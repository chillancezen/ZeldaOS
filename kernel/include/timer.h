/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _TIMER_H
#define _TIMER_H
#include <lib/include/heap_sort.h>
#include <kernel/include/jiffies.h>

enum timer_state {
    timer_state_idle = 0,
    timer_state_scheduled,
};

struct timer_entry {
    struct binary_tree_node node;
    enum timer_state state;
    uint64_t time_to_expire;
    void (*callback)(struct timer_entry * entry, void * priv);
    void * priv;
};

int32_t
timer_detached(struct timer_entry * timer);

void
timer_init(void);

void
register_timer(struct timer_entry * entry);

void
cancel_timer(struct timer_entry * entry);

void
schedule_timer(void);
#endif
