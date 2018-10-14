/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _MEMFS_H
#define _MEMFS_H
#include <lib/include/types.h>
#include <lib/include/list.h>
#include <memory/include/paging.h>
#if 1
    #define MEM_BLOCK_SIZE PAGE_SIZE
    #define MEM_BLOCK_ALIGN PAGE_SIZE
#else
    #define MEM_BLOCK_SIZE 512
    #define MEM_BLOCK_ALIGN 1
#endif

struct mem_block_hdr {
    struct list_elem list;
    // If the block is the head, nr_used store the size of the blocks
    uint32_t nr_used;
    uint8_t content[0];
}__attribute__((packed));

#define BLOCK_AVAIL_SIZE (MEM_BLOCK_SIZE - sizeof(struct mem_block_hdr))

struct mem_file {
    uint32_t total_size;
    uint32_t nr_blocks;
    struct mem_block_hdr block_head;
};

void
put_mem_block(struct mem_block_hdr * hdr);

int32_t
mem_block_raw_write_sequential(struct mem_block_hdr * hdr,
    void * buffer,
    int size);

int32_t
mem_block_raw_write_random(struct mem_block_hdr * hdr,
    uint32_t offset,
    void * buffer,
    int size);

int32_t
mem_block_raw_read(struct mem_block_hdr * hdr,
    uint32_t offset,
    void * buffer,
    int size);

int32_t
mem_block_raw_truncate(struct mem_block_hdr * hdr,
    uint32_t offset);

void
mem_block_raw_reclaim(struct mem_block_hdr *hdr);

void dump_mem_blocks(struct mem_block_hdr * hdr);
void
memfs_init(void);

#endif
