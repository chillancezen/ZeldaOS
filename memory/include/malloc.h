/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _MALLOC_H
#define _MALLOC_H
#include <lib/include/types.h>
/*
memory layout for allocated chunk
--------   +-----------+
 |         |           | malloc_header
 |         |           |
 |         +-+---------+
 |         | ^         |
 |         | |         |
 |         | |padding  |
 |         | |         |
 |         +-----------+
 |         |           | padding_header
 |         +-----------+
 |         |           | <-----------user ptr
 |size     |           |
 |         |           |
 |         |           |
---------  +-----------+
for free chunk, the padding header is not present.
*/
#define MALLOC_MAGIC 0x5555
#define RECYCLE_BIN_SIZE (12 + 1)
#define MISC_BIN (RECYCLE_BIN_SIZE - 1)
/*
 * the select policy is best fit
 * with buddy system to recycle the memory block to free, when 
 * a memory block is to be recycled, it is coalesced with the block
 * via next_block pointer.
 */

struct malloc_header {
    uint32_t prev;
    uint32_t next;
    uint32_t size;
    uint16_t padding;
    uint16_t free:1;
    uint16_t magic:15;
}__attribute__((packed));



struct padding_header {
    uint16_t padding;
    uint16_t free:1;
    uint16_t magic:15;
}__attribute__((packed));


#define LENGTH_TO_RECYCLE_BIN_INDEX(len) ({\
    int32_t _bin_len[RECYCLE_BIN_SIZE - 1] = { \
        0x10, 0x20, 0x40, 0x80, \
        0x100, 0x200, 0x400, 0x800, \
        0x1000, 0x2000, 0x4000, 0x8000}; \
    int32_t _idx = 0; \
    for(; _idx < RECYCLE_BIN_SIZE - 1; _idx++) \
        if ((len) <= _bin_len[_idx]) \
            break; \
    _idx; \
})
void dump_recycle_bins(void);
void malloc_init(void);
#endif
