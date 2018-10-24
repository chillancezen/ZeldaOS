/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/timer.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <kernel/include/jiffies.h>

struct heap_stub timer_heap;

static int32_t
timer_compare(struct binary_tree_node * node0, struct binary_tree_node * node1)
{
    struct timer_entry * timer0 = NULL;
    struct timer_entry * timer1 = NULL;
    timer0 = CONTAINER_OF(node0, struct timer_entry, node);
    timer1 = CONTAINER_OF(node1, struct timer_entry, node);
    return timer0->time_to_expire < timer1->time_to_expire ?
        -1 : timer0->time_to_expire > timer1->time_to_expire ? 1 : 0;
}

void
register_timer(struct timer_entry * entry)
{
    ASSERT(!entry->node.left);
    ASSERT(!entry->node.right);
    ASSERT(!entry->node.parent);
    entry->state = timer_state_scheduled;
    attach_heap_node(&timer_heap, &entry->node, timer_compare);
    LOG_DEBUG("Registered timer entry:0x%x\n", entry);
}

void
cancel_timer(struct timer_entry * entry)
{
    delete_heap_node(&timer_heap, &entry->node, timer_compare);
    entry->state = timer_state_idle;
    ASSERT(!entry->node.left);
    ASSERT(!entry->node.right);
    ASSERT(!entry->node.parent);
    LOG_DEBUG("Cancel timer entry:0x%x\n", entry);
}
void
schedule_timer(void)
{
    struct timer_entry * timer;
    struct binary_tree_node * current_node;
    while ((current_node = timer_heap.root)) {
        timer = CONTAINER_OF(current_node, struct timer_entry, node);
        ASSERT(timer->state == timer_state_scheduled);
        if (timer->time_to_expire > jiffies)
            break;
        current_node = detach_heap_node(&timer_heap, timer_compare);
        ASSERT(current_node);
        timer = CONTAINER_OF(current_node, struct timer_entry, node);
        ASSERT(timer->time_to_expire <= jiffies);
        ASSERT(timer->state == timer_state_scheduled);
        ASSERT(timer->callback);
        timer->callback(timer, timer->priv);
        LOG_TRIVIA("Scheduled timer:0x%x\n", timer);
    }
}
void
timer_init(void)
{
    memset(&timer_heap, 0x0, sizeof(timer_heap));
}
