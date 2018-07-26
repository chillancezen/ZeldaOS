/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PAGING_H
#define _PAGING_H
#include<memory/include/physical_memory.h>
/*
 * page directory entry in 32-bit mode
 */
struct pde32 {
    uint32_t present:1;
    uint32_t write_permission:1;
    uint32_t supervisor_permission:1;
    uint32_t page_writethrough:1;
    uint32_t page_cachedisable:1;
    uint32_t accessed:1;
    uint32_t reserved0:1;
    uint32_t page_size:1;
    uint32_t reserved1:4;
    uint32_t pt_frame:20;
}__attribute__((packed));

/*
 * page table entry in 32-bit
 */
struct pte32 {
    uint32_t present:1;
    uint32_t write_permission:1;
    uint32_t supervisor_permission:1;
    uint32_t page_writethrough:1;
    uint32_t page_cachedisable:1;
    uint32_t accessed:1;
    uint32_t dirty:1;
    uint32_t pat:1;// page attribute table
    uint32_t global:1;
    uint32_t reserved:3;
    uint32_t pg_frame:20;
}__attribute__((packed));

uint32_t
create_pte32(uint32_t write_permission,
    uint32_t supervisor_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable,
    uint32_t pg_frame);

uint32_t
create_pde32(uint32_t write_permission,
    uint32_t supervisor_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable,
    uint32_t pt_frame);


#define PDE32_TO_DWORD(entry) (*(uint32_t*)(entry))
#define PTE32_TO_DWORD(entry) PDE32_TO_DWORD(entry)

#define PDE32_PTR(addr) ((struct pde32*)(addr))
#define PTE32_PTR(addr) ((struct pte32*)(addr))

#define FREE_PAGE_BITMAP_SIZE  ((1 << 20) >> 3)
#define PHYSICAL_MEMORY_TO_PAGE_FRAME(mem) (((uint32_t)(mem)) >> 12)

#define AT_BYTE(fn) ((fn) >> 3)
#define AT_BIT(fn) ((fn) & 0x7)

#define INVALID_PAGE 0xffffffff

#define IS_PAGE_FREE(pg_fn) ({\
    uint32_t _byte_index = AT_BYTE((pg_fn)); \
    uint8_t _bit_index = AT_BIT((pg_fn)); \
    !((free_page_bitmap[_byte_index] >> _bit_index) & 0x1); \
})

#define MAKR_PAGE_AS_FREE(pg_fn) {\
    uint32_t _byte_index = AT_BYTE((pg_fn)); \
    uint8_t _bit_index = AT_BIT((pg_fn)); \
    uint8_t _mask = 1 << _bit_index; \
    _mask = ~_mask; \
    free_page_bitmap[_byte_index] &= _mask; \
}

#define MARK_PAGE_AS_OCCUPIED(pg_fn) { \
    uint32_t _byte_index = AT_BYTE((pg_fn)); \
    uint8_t _bit_index = AT_BIT((pg_fn)); \
    uint8_t _mask = 1 << _bit_index; \
    free_page_bitmap[_byte_index] |= _mask; \
}

/*
 * deprecated macro to search free pages, as it only can search one free page
 * every time
 */
#define SEARCH_FREE_PAGE(start_addr, mem_boundary) ({\
    uint32_t _addr = (uint32_t)(start_addr); \
    uint32_t _target = INVALID_PAGE; \
    for (; _addr < (uint32_t)(mem_boundary); _addr += PAGE_SIZE) { \
        if (IS_PAGE_FREE(_addr)) { \
            _target = _addr; \
            break; \
        } \
    } \
    if (_target != INVALID_PAGE) \
        _target = (uint32_t)(_target & (~PAGE_MASK)); \
    _target; \
})

#define UPPER_MEMORY_PAGE_INDEX  PHYSICAL_MEMORY_TO_PAGE_FRAME(0x100000)

#define PAGE_PERMISSION_READ_ONLY 0x0
#define PAGE_PERMISSION_READ_WRITE 0x1

#define PAGE_PERMISSION_USER 0x1
#define PAGE_PERMISSION_SUPERVISOR 0x0

#define PAGE_WRITETHROUGH 0x1
#define PAGE_WRITEBACK 0x0

#define PAGE_CACHE_DISABLED 0x1
#define PAGE_CACHE_ENABLED 0x0

uint32_t get_pages(int nr_pages);
uint32_t get_page(void);
void free_pages(uint32_t pg_addr, int nr_pages);
void free_page(uint32_t pg_addr);
void paging_init(void);

#endif
