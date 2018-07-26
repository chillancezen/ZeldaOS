/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>


static uint8_t free_page_bitmap[FREE_PAGE_BITMAP_SIZE];


/*
 * Function to create a 4k page table entry
 * write_permission: in [PAGE_PERMISSION_READ_ONLY, PAGE_PERMISSION_READ_WRITE]
 * supervisor permission: in [PAGE_PERMISSION_USER, PAGE_PERMISSION_SUPERVISOR]
 * page_writethrough: in [PAGE_WRITETHROUGH, PAGE_WRITEBACK]
 * page_cachedisable: in [PAGE_CACHE_DISABLED, PAGE_CACHE_ENABLED]
 * pg_frame: the page table physical address, the value is not necessary to be
 * 4K aligned.
 */
uint32_t
create_pte32(uint32_t write_permission,
    uint32_t supervisor_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable,
    uint32_t pg_frame)
{
    struct pte32 pte;
    memset(&pte, 0x0, sizeof(pte));
    pte.present = 1;
    pte.write_permission = write_permission;
    pte.supervisor_permission = supervisor_permission;
    pte.page_writethrough = page_writethrough;
    pte.page_cachedisable = page_cachedisable;
    pte.pg_frame = pg_frame >> 12;
    return PTE32_TO_DWORD(&pte);
}

/*
 * Function to create a page directory entry
 * these parameters are self-explanatory as with create_pte32()
 */
uint32_t
create_pde32(uint32_t write_permission,
    uint32_t supervisor_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable,
    uint32_t pt_frame)
{
    struct pde32 pde;
    memset(&pde, 0x0, sizeof(pde));
    pde.present = 1;
    pde.write_permission = write_permission;
    pde.supervisor_permission = supervisor_permission;
    pde.page_writethrough = page_writethrough;
    pde.page_cachedisable = page_cachedisable;
    pde.pt_frame = pt_frame >> 12;
    return PTE32_TO_DWORD(&pde);
}

void
paging_init(void)
{
    memset(free_page_bitmap, 0x0, sizeof(free_page_bitmap));
    printk("entry:%x\n", create_pde32(1,1,0,1,0x1234000));
}
