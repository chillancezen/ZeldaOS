/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/ring.h>
#include <kernel/include/printk.h>


#if defined(INLINE_TEST)
#include <lib/include/string.h>

__attribute__((constructor)) void
ring_inline_test(void)
{
    uint8_t foo[sizeof(struct ring) + 64];
    struct ring * ring = (struct ring *)foo;
    memset(foo, 0x0, sizeof(foo));
    ring->front = 0;
    ring->rear = 0;
    ring->ring_size = 64;
    ASSERT(!ring_full(ring));
    ASSERT(ring_empty(ring));
    {
        int idx = 0;
        uint8_t value = 0xff;
        ASSERT(ring_enqueue(ring, value) == 1);
        ASSERT(!ring_full(ring));
        ASSERT(!ring_empty(ring));
        for (idx = 0; idx < 62; idx++) {
            value = idx;
            ASSERT(ring_enqueue(ring, value) == 1);
            ASSERT(!ring_empty(ring));
        }
        ASSERT(!ring_empty(ring));
        ASSERT(ring_full(ring));
        ASSERT(!ring_enqueue(ring, value));
        ASSERT(ring_dequeue(ring, &value) == 1);
        ASSERT(value == 0xff);
        ASSERT(!ring_full(ring));
        ASSERT(!ring_empty(ring));
        for (idx = 0; idx < 62; idx++) {
            ASSERT(ring_dequeue(ring, &value) == 1);
            ASSERT(value == idx);
            ASSERT(!ring_full(ring));
        }
        ASSERT(ring_empty(ring));
        ASSERT(ring_dequeue(ring, &value) == 0);
    }
}
#endif
