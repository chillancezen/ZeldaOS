/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
static struct malloc_header _recycle_bins[RECYCLE_BIN_SIZE];


void *
__malloc(struct malloc_header * hdr, int len, int align)
{
    ASSERT(hdr->free);
    ASSERT(hdr->magic == MALLOC_MAGIC);
    /*
     * 1st step: ensure the memory chunk is big enough to contain
     * the required user length plus metadata.
     */
    if((len +
        sizeof(struct malloc_header) +
        sizeof(struct padding_header)) > 
        hdr->size)
        return NULL;
    
    /*
     * 2nd step: determine the extra space for alignment
     */
    return NULL;
}

void *
malloc_align(int len, int align)
{
    int least_len = len +
         sizeof(struct malloc_header) +
         sizeof(struct padding_header);
    int bin_idx = LENGTH_TO_RECYCLE_BIN_INDEX(least_len);

    return __malloc(&_recycle_bins[bin_idx], len, align);
}
void
dump_recycle_bins(void)
{
    int idx = 0;
    struct malloc_header *hdr;
    char * hint[13] = {
        "bin-16", "bin-32", "bin-64", "bin-128",
        "bin-256", "bin-512", "bin-1024", "bin-2048",
        "bin-4096", "bin-8192", "bin-16384", "bin-32768",
        "bin-misc"
    };
    LOG_INFO("Dump malloc recycle bins:\n");
    for(; idx < RECYCLE_BIN_SIZE; idx++) {
        if(!_recycle_bins[idx].next)
            continue;
        for(hdr = (struct malloc_header *)_recycle_bins[idx].next;
            hdr;
            hdr = (struct malloc_header *)hdr->next) {
            LOG_INFO("%s: addr:0x%x size:%d\n",
                hint[idx],
                (uint32_t)hdr,
                hdr->size);
        }
    }
}
void
malloc_init(void)
{
    struct malloc_header * _malloc_hdr = NULL;
    memset(_recycle_bins, 0x0, sizeof(_recycle_bins));
    /*
     *put the whole kernel heap into last recycle bin
     */
    _malloc_hdr = (struct malloc_header *)KERNEL_HEAP_BOTTOM;
    memset(_malloc_hdr, 0x0, sizeof(struct malloc_header));
    _malloc_hdr->prev = 0;
    _malloc_hdr->next = 0;
    _malloc_hdr->size = KERNEL_HEAP_TOP - KERNEL_HEAP_BOTTOM;
    _malloc_hdr->padding = 0;
    _malloc_hdr->free = 1;
    _malloc_hdr->magic = MALLOC_MAGIC;

    _malloc_hdr->prev = (uint32_t)&_recycle_bins[MISC_BIN];
    _recycle_bins[MISC_BIN].next  = KERNEL_HEAP_BOTTOM;
}

