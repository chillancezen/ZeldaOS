/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/memfs.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>

static struct mem_block_hdr *
get_mem_block(void)
{
    struct mem_block_hdr * hdr  = (struct mem_block_hdr *)malloc_align(
        MEM_BLOCK_SIZE, MEM_BLOCK_ALIGN);
    if (hdr) {
        memset(hdr, 0x0, sizeof(struct mem_block_hdr));
        LOG_TRIVIA("Allocate memory block:0x%x\n", hdr);
    } else {
        LOG_TRIVIA("Failed to allocate memory block\n");
    }
    return hdr;
}

static void
put_mem_block(struct mem_block_hdr * hdr)
{
    // Make sure the block is free (not in any list)
    ASSERT(!hdr->list.next);
    ASSERT(!hdr->list.prev);
    LOG_TRIVIA("Deallocate memory block:0x%x\n", hdr);
    free(hdr);
}

/*
 * The raw interface to write buffer sequentially into memory block,
 * always return the number of bytes, note that 0 indicate out of resource..
 */
int32_t
mem_block_raw_write_sequential(struct mem_block_hdr * hdr,
    void * buffer,
    int size)
{
    int32_t nr_bytes_to_write = 0;
    int32_t left = 0;
    int32_t result = 0;
    struct list_elem * last_elem = NULL;
    struct mem_block_hdr * _block = NULL;
    ASSERT(size >= 0);
    if (size == 0) {
        return 0;
    }

    left = size;
    while (left > 0) {
        // Inspect the last block.
        last_elem = list_last_elem(&hdr->list);
        if (!last_elem) {
            _block = get_mem_block();
            if (!_block)
                break;
            list_append(&hdr->list, &_block->list);
            last_elem = list_last_elem(&hdr->list);
        }
        ASSERT(last_elem);
        // Check the capacity of the last block
        _block = CONTAINER_OF(last_elem, struct mem_block_hdr, list);
        nr_bytes_to_write = BLOCK_AVAIL_SIZE - _block->nr_used;
        ASSERT(nr_bytes_to_write >= 0);
        if (!nr_bytes_to_write) {
            _block = get_mem_block();
            if (!_block)
                break;
            list_append(&hdr->list, &_block->list);
            last_elem = list_last_elem(&hdr->list);
            _block = CONTAINER_OF(last_elem, struct mem_block_hdr, list);
            nr_bytes_to_write = BLOCK_AVAIL_SIZE - _block->nr_used;
        }
        ASSERT(nr_bytes_to_write > 0);
        // Write block and update metadata
        nr_bytes_to_write = MIN(nr_bytes_to_write, left);
        memcpy(_block->content + _block->nr_used,
            buffer + result,
            nr_bytes_to_write);
        _block->nr_used += nr_bytes_to_write;
        ASSERT(_block->nr_used <= BLOCK_AVAIL_SIZE);
        result += nr_bytes_to_write;
        left -= nr_bytes_to_write;
    }
    return result;
}
/*
 * The raw interface to randomly write buffer into memory block,
 * return the number of the bytes (including ZERO) which have been written,
 * XXX: note the function is expensive because it will do a traversal to
 * determine the bytes to skip.
 */
int32_t
mem_block_raw_write_random(struct mem_block_hdr * hdr,
    uint32_t offset,
    void * buffer,
    int size)
{
    int32_t left = size;
    int32_t result = 0;
    int32_t nr_bytes_to_write = 0;
    int32_t block_bytes_ignored = 0;
    int32_t offset_left = offset;
    int32_t block_iptr = 0;
    struct list_elem * _list = NULL;
    struct mem_block_hdr * _block = NULL;
    struct mem_block_hdr * current_block = NULL;
    // Try to skip the bytes within the `offset`
    LIST_FOREACH_START(&hdr->list, _list) {
        block_iptr = 0;
        _block = CONTAINER_OF(_list, struct mem_block_hdr, list);
        if (offset_left) {
            block_bytes_ignored = MIN(offset_left, _block->nr_used);
            offset_left -= block_bytes_ignored;
            block_iptr += block_bytes_ignored;
        }
        if (!offset_left) {
            current_block = _block;
            break;
        }
    }
    LIST_FOREACH_END();
    while (offset_left > 0) {
        _block = get_mem_block();
        if (!_block)
            break;
        block_bytes_ignored = MIN(BLOCK_AVAIL_SIZE, offset_left);
        _block->nr_used = block_bytes_ignored;
        offset_left -= block_bytes_ignored;
        list_append(&hdr->list, &_block->list);
        if (!offset_left) {
            block_iptr = block_bytes_ignored;
            current_block = _block;
            break;
        }
    }
    if (offset_left) {
        return result;
    }
    // Start to write the buffer into memory block
    while (left > 0) {
        if (current_block) {
            nr_bytes_to_write = BLOCK_AVAIL_SIZE - block_iptr;
            nr_bytes_to_write = MIN(nr_bytes_to_write, left);
            memcpy(current_block->content + block_iptr,
                result + (uint8_t *)buffer,
                nr_bytes_to_write);
            left -= nr_bytes_to_write;
            result += nr_bytes_to_write;
            if (current_block->nr_used < (block_iptr + nr_bytes_to_write))
                current_block->nr_used = block_iptr + nr_bytes_to_write;
            if (!left)
                break;
            _list = current_block->list.next;
            current_block = _list ?
                CONTAINER_OF(_list, struct mem_block_hdr, list) :
                NULL;
            // XXX:reset block_iptr forever
            block_iptr = 0x0;
        } else {
            _block = get_mem_block();
            if (!_block)
                break;
            _block->nr_used = 0x0;
            block_iptr = 0x0;
            list_append(&hdr->list, & _block->list);
            current_block = _block;
        }
    }
    return result;
}

int32_t
mem_block_raw_read(struct mem_block_hdr * hdr,
    uint32_t offset,
    void * buffer,
    int size)
{
    int32_t offset_left = offset;
    int32_t left = size;
    int32_t result = 0;
    int32_t nr_bytes_to_read = 0;
    int32_t block_iptr;
    int32_t block_left;
    int32_t block_bytes_ignored;
    struct list_elem * _list;
    struct mem_block_hdr * _block;
    LIST_FOREACH_START(&hdr->list, _list) {
        ASSERT(left >= 0);
        if (!left)
            break;
        _block = CONTAINER_OF(_list, struct mem_block_hdr, list);
        block_iptr = 0x0;
        block_left = _block->nr_used;
        // Skip numbrt of bytes as specified in `offset`
        if (offset_left) {
            block_bytes_ignored = MIN(block_left, offset_left);
            block_iptr += block_bytes_ignored;
            block_left -= block_bytes_ignored;
            offset_left -= block_bytes_ignored;
        }
        if (!block_left)
            continue;
        nr_bytes_to_read = MIN(block_left, left);
        // This prevents no NULL block appears in the block list.
        ASSERT(nr_bytes_to_read);
        memcpy(((uint8_t *)buffer) + result,
            _block->content + block_iptr,
            nr_bytes_to_read);
        result += nr_bytes_to_read;
        left -= nr_bytes_to_read;
    }
    LIST_FOREACH_END();
    return result;
}

/*
 * Extend or Shrink size of the memory blocks to `oiffset`
 * return OK once successful, otherwise a negative integer is returned.
 */
int32_t
mem_block_raw_truncate(struct mem_block_hdr * hdr,
    uint32_t offset)
{
    int32_t offset_left = offset;
    int32_t block_iptr = 0x0;
    int32_t block_bytes_ignored = 0x0;
    struct list_elem * _list;
    struct mem_block_hdr * next_block;
    struct mem_block_hdr * _block;
    struct mem_block_hdr * current_block = NULL;
    LIST_FOREACH_START(&hdr->list, _list) {
        block_iptr = 0x0;
        _block = CONTAINER_OF(_list, struct mem_block_hdr, list);
        if (offset_left) {
            // XXX: here we should not use _block->nr_used to determine the 
            // bytes to skip, instead we use BLOCK_AVAIL_SIZE
            block_bytes_ignored = MIN(offset_left, BLOCK_AVAIL_SIZE);
            offset_left -= block_bytes_ignored;
            block_iptr = block_bytes_ignored;
        }
        if (!offset_left) {
            current_block = _block;
            break;
        }
    }
    LIST_FOREACH_END();
    // Extend memory blocks if `offset_left` still is larger than 0
    while (offset_left > 0) {
        _block = get_mem_block();
        if (!_block) {
            return -ERR_OUT_OF_MEMORY;
        }
        block_bytes_ignored = MIN(BLOCK_AVAIL_SIZE, offset_left);
        _block->nr_used = block_bytes_ignored;
        offset_left -= block_bytes_ignored;
        list_append(&hdr->list, &_block->list);
        if (!offset_left) {
            block_iptr = block_bytes_ignored;
            current_block = _block;
            break;
        }
    }
    if (current_block) {
        current_block->nr_used = block_iptr;
        if (current_block->nr_used) {
            current_block = current_block->list.next ?
                CONTAINER_OF(current_block->list.next,
                    struct mem_block_hdr,
                    list) :
                NULL;
        }
    }
    // Shrink memeory blocks
    while (current_block) {
        next_block = current_block->list.next ?
            CONTAINER_OF(current_block->list.next, struct mem_block_hdr, list) :
            NULL;
        list_delete(&hdr->list, &current_block->list);
        put_mem_block(current_block);
        current_block = next_block;
    }
    return OK; 
}
/*
 * Delete the file's successive memory blocks
 */
void
mem_block_raw_reclaim(struct mem_block_hdr *hdr)
{
    struct list_elem * _list;
    struct mem_block_hdr * _block;
    while(!list_empty(&hdr->list)) {
        _list = list_fetch(&hdr->list);
        _block = CONTAINER_OF(_list, struct mem_block_hdr, list);
        free(_block);
    }
}

void
dump_mem_blocks(struct mem_block_hdr * hdr)
{
    struct list_elem * _list;
    struct mem_block_hdr * _block;
    int iptr = 0;
    LOG_DEBUG("Dump memory blocks for header:0x%x\n", hdr);
    LIST_FOREACH_START(&hdr->list, _list) {
        _block = CONTAINER_OF(_list, struct mem_block_hdr, list);
        LOG_DEBUG("   %d.0x%x length:%d\n", iptr++,  _block, _block->nr_used);
    }
    LIST_FOREACH_END();
}
