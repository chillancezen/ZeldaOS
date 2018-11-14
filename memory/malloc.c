/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/kernel.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <memory/include/paging.h>

static struct malloc_header _recycle_bins[RECYCLE_BIN_SIZE];

void __free(struct malloc_header * hdr);

#define VALIDATE_ALIGNMENT(align) {\
    int32_t _idx = 0; \
    ASSERT((align)); \
    for(; _idx < 32; _idx++)  \
        if((align) & (1 << _idx)) { \
            ASSERT((align) == (1 << _idx)); \
            break; \
        }\
}

#define NEXT_BLOCK(hdr) ({\
    struct malloc_header * _hdr = (hdr); \
    struct malloc_header * _hdr1 = NULL; \
    if ((hdr->size + (uint32_t)_hdr) < KERNEL_HEAP_TOP) \
        _hdr1 = (struct malloc_header *)(hdr->size + (uint32_t)_hdr); \
    _hdr1; \
})

void *
__malloc(struct malloc_header * hdr, int len, int align)
{

    uint32_t mask = align -1;
    uint32_t usr_ptr = 0;
    uint32_t padding = 0;
    uint32_t left = 0;
    struct malloc_header * prev;
    struct malloc_header * next;
    struct malloc_header * hdr1 = NULL;
    struct padding_header * padding_hdr = NULL;
    ASSERT(hdr->free);
    ASSERT(hdr->magic == MALLOC_MAGIC);
    VALIDATE_ALIGNMENT(align);
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
    usr_ptr = ((uint32_t)hdr) +
        sizeof(struct malloc_header) +
        sizeof(struct padding_header);
    if(usr_ptr & mask) {
        padding = align - ((uint32_t)(usr_ptr & mask));
    }
    usr_ptr += padding;
    ASSERT(!(usr_ptr & mask));
    /*
     * 2.5th step, re-check the length to prevent overflow
     */
    if((len +
        sizeof(struct malloc_header) +
        sizeof(struct padding_header) +
        padding) >
        hdr->size)
        return NULL;
    /*
     * 3rd step: calculate the left bytes 
     * and decide whether to split into halves. 
     */
    left = hdr->size -
        sizeof(struct malloc_header) -
        sizeof(struct padding_header) -
        padding -
        len;
    /*
     * 4th step: detach the malloc_header from the bin list
     */
    next = (struct malloc_header *)hdr->next;
    prev = (struct malloc_header *)hdr->prev;
    ASSERT(prev);
    prev->next = hdr->next;
    if(next)
        next->prev = hdr->prev;

    if(left > (sizeof(struct malloc_header) + sizeof(struct padding_header))) {
        hdr1 = (struct malloc_header*)(((uint32_t)hdr) + hdr->size - left);
        hdr1->magic = MALLOC_MAGIC;
        hdr1->free = 1;
        hdr1->padding = 0;
        hdr1->size = left;
        hdr1->prev = 0;
        hdr1->next = 0;
        hdr->size -=left;
    }
    /*
     * 5th step: prepare allodated memory chunk.
     */
    hdr->prev = 0;
    hdr->next = 0;
    hdr->padding = padding;
    hdr->free = 0;
    padding_hdr = (struct padding_header *)
        (((uint32_t)hdr) + sizeof(struct malloc_header) + padding);
    padding_hdr->padding = padding;
    padding_hdr->free = 0;
    padding_hdr->magic = MALLOC_MAGIC;
    /*
     *6th step: free the splited bottom half
     */
    __free(hdr1);
    return (void *)usr_ptr;
}

void *
malloc_align(int len, int align)
{
    uint32_t free_chunk_addr;
    void * user_ptr = NULL;
    int bin_idx = LENGTH_TO_RECYCLE_BIN_INDEX(len);
    for (; bin_idx < RECYCLE_BIN_SIZE; bin_idx++) {
        for (free_chunk_addr = _recycle_bins[bin_idx].next;
            free_chunk_addr;
            free_chunk_addr = ((struct malloc_header *)free_chunk_addr)->next) {
            user_ptr = __malloc((struct malloc_header *)free_chunk_addr,
                len,
                align);
            if (user_ptr) {
                LOG_TRIVIA("memory allocation: [size:%d align:%d] as 0x%x\n",
                    len, align, user_ptr);
                return user_ptr;
            }
        }
    }
    LOG_TRIVIA("memory allocation: [size:%d align:%d] as 0x%x\n",
        len, align, NULL);
    return NULL;
}

void
__free(struct malloc_header * hdr)
{
    int32_t bin_index = 0;
    int32_t avail_size = 0;
    struct malloc_header * next_hdr = NEXT_BLOCK(hdr);
    struct malloc_header * _ptr = NULL;
    ASSERT(hdr->size >= sizeof(struct malloc_header));
    if (next_hdr && next_hdr->free) {
        ASSERT(next_hdr->magic == MALLOC_MAGIC);
        struct malloc_header * _prev = (struct malloc_header *)next_hdr->prev;
        struct malloc_header * _next = (struct malloc_header *)next_hdr->next;
        ASSERT(_prev);
        _prev->next = next_hdr->next;
        if(_next)
            _next->prev = next_hdr->prev;
        hdr->size += next_hdr->size;
    }
    avail_size = hdr->size -
        sizeof(struct malloc_header) -
        sizeof(struct padding_header);
    bin_index = LENGTH_TO_RECYCLE_BIN_INDEX(avail_size);
    for(_ptr = (struct malloc_header *)_recycle_bins[bin_index].next;
        _ptr;
        _ptr = (struct malloc_header *)_ptr->next) {
        if(_ptr->size >= hdr->size)
            break;
    }
    hdr->free = 1;
    hdr->padding = 0;
    if (_ptr) {
        hdr->prev = _ptr->prev;
        hdr->next = (uint32_t)_ptr;
        _ptr->prev = (uint32_t)hdr;
        ((struct malloc_header *)(hdr->prev))->next = (uint32_t)hdr;
    } else {
        _ptr = (struct malloc_header *)&_recycle_bins[bin_index];
        for(; _ptr->next; _ptr = (struct malloc_header *)_ptr->next);
        hdr->prev = (uint32_t)_ptr;
        hdr->next = (uint32_t)NULL;
        _ptr->next = (uint32_t)hdr;
    }
}
void
free(void * mem)
{
    struct malloc_header * malloc_hdr = NULL;
    struct padding_header * padding_hdr = (struct padding_header *)
        (((uint32_t)mem) - sizeof(struct padding_header));
    ASSERT(padding_hdr->magic == MALLOC_MAGIC);
    malloc_hdr = (struct malloc_header *)(((uint32_t)padding_hdr) -
        padding_hdr->padding - sizeof(struct malloc_header));
    LOG_TRIVIA("memory free: 0x%x is_free:%d\n", mem, malloc_hdr->free);
    if (malloc_hdr->free)
        return;
    __free(malloc_hdr);
}

void *
malloc(int len)
{
    return malloc_align(len, 1);
}

void *
malloc_align_mapped(int len, int align)
{
    uint32_t * kernel_page_drectory = (uint32_t *)get_kernel_page_directory();
    uint32_t addr = (uint32_t)malloc_align(len, align);
    uint32_t virt_addr = 0;
    if (addr) {
        for (virt_addr = addr;
            virt_addr < (addr + (uint32_t)len);
            virt_addr += PAGE_SIZE) {
            if (page_present(kernel_page_drectory, virt_addr) != OK) {
                kernel_map_page(virt_addr,
                    ({uint32_t __addr = get_page();
                        ASSERT(__addr);
                        __addr;}),
                    PAGE_PERMISSION_READ_WRITE,
                    PAGE_WRITEBACK,
                    PAGE_CACHE_ENABLED);
                ASSERT(!page_present(kernel_page_drectory, virt_addr));
            }
        }
    }
    return (void *)addr;
}
void *
malloc_mapped(int len)
{
    return malloc_align_mapped(len, 1);
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
/*
 * Allocate a memory segment from caller's stack.
 * note this function will do not do any stack overflow check.
 * XXX: no alignment is enforced as the returned address is 4 bytes aligned.
 * there is no need to free the allocated memory explicitly: the caller will do
 * that automatically. 
 */
void *
stack_alloc(uint32_t size)
{
    uint32_t new_stack = 0x0;
    uint32_t allocated_addr = 0x0;
    uint32_t current_ebp = 0x0;
    asm volatile("movl %%ebp, %0;"
        :"=m"(current_ebp)
        :
        :"memory");
    ASSERT(!(current_ebp & 0x3));
    allocated_addr = current_ebp + 8;
    size = (size & 0x3) ? (size & (~0x3)) - 4 : size;

    new_stack = allocated_addr - size - 8;
    ASSERT(!(new_stack & 0x3));

    // Copy EIP and ESP of Last Frame
    *(uint32_t *)new_stack = *(uint32_t *)current_ebp;
    *(uint32_t *)(new_stack + 4) = *(uint32_t *)(current_ebp + 4);

    asm volatile("movl %%ebx, %%ebp;"
        "movl %%ebp, %%esp;" // equivalent to `leave` instruction
        "popl %%ebp;"
        "ret;"
        :
        :"a"(allocated_addr), "b"(new_stack)
        :"memory");

    ASSERT(0);
    return NULL;
}

#if defined(INLINE_TEST)
static void
malloc_test(void)
{
    void * ptr = NULL;
    struct padding_header * padding;
    struct malloc_header * hdr;
    ptr = malloc_align(1, 1024);
    ASSERT(!(1023 & (uint32_t)ptr));
    padding = ((struct padding_header *)ptr) - 1;
    ASSERT(!padding->free);
    ASSERT(padding->magic == MALLOC_MAGIC);

    hdr = (struct malloc_header *)(((uint32_t)padding) -
        padding->padding -
        sizeof(struct malloc_header));
    ASSERT(!hdr->free);
    ASSERT(hdr->magic == MALLOC_MAGIC);
    ASSERT(hdr->padding == padding->padding);
    free(ptr);
    ASSERT(hdr->free);
    dump_recycle_bins();
}
#endif
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
#if defined(INLINE_TEST)
    malloc_test();
#endif
}

