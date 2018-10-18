/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>


static uint8_t free_page_bitmap[FREE_PAGE_BITMAP_SIZE];
/*
 * The bit map records the free pages in the page inventory
 */
#define FREE_BASE_PAGE_BITMAP_SIZE \
    (((PAGE_SPACE_TOP - PAGE_SPACE_BOTTOM) >> 12) >> 3)
static uint8_t free_base_page_bitmap[FREE_BASE_PAGE_BITMAP_SIZE];
/*
 * the kernel page directory is the PDBR address.
 * NOTE the kernel_page_directory is not mapped into kernel virtual adddress
 * space, they are only accessable in page fault handler by disabling paging
 * the benefit is : the kernel even does not have the right to access the
 * page directory/table, and simplify the kernel virtual address layout
 */
static uint32_t * kernel_page_directory;

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
/*
 *Get a free page from PageInventory VMA. usually these pages are to construct
 *page directory/table for both kernnel and userspace.
 *return 0 if no available page is found.
 * FIXED: create a *FAST* method to seach free base page ... as it makes 
 * significant sense.
 */
static uint32_t fast_base_page_addr = 0;
uint32_t
get_base_page_fast(void)
{
    int32_t offset;
    int32_t byte_idx = 0;
    int32_t bit_idx = 0;
    uint8_t bit_set_mask = 0;
    uint32_t target_page = 0;
    if (fast_base_page_addr < PAGE_SPACE_BOTTOM ||
        fast_base_page_addr >= PAGE_SPACE_TOP)
        fast_base_page_addr = PAGE_SPACE_BOTTOM;
    for(; fast_base_page_addr < PAGE_SPACE_TOP;
        fast_base_page_addr += PAGE_SIZE) {
        offset = fast_base_page_addr - PAGE_SPACE_BOTTOM;
        offset = offset >> 12;
        byte_idx = offset >> 3;
        bit_idx = offset & 0x7;
        bit_set_mask = 1 << bit_idx;
        if(free_base_page_bitmap[byte_idx] & bit_set_mask)
            continue;
        free_base_page_bitmap[byte_idx] |= bit_set_mask;
        target_page = fast_base_page_addr;
        break;
    }
    return target_page;
}
uint32_t
get_base_page_slow(void)
{
    uint32_t _page = PAGE_SPACE_BOTTOM;
    int32_t offset;
    int32_t byte_idx = 0;
    int32_t bit_idx = 0;
    uint8_t bit_set_mask = 0;
    for(; _page < PAGE_SPACE_TOP; _page += PAGE_SIZE) {
        offset = _page - PAGE_SPACE_BOTTOM;
        offset = offset >> 12;
        byte_idx = offset >> 3;
        bit_idx = offset & 0x7;
        bit_set_mask = 1 << bit_idx;
        if (free_base_page_bitmap[byte_idx] & bit_set_mask)
            continue;
        free_base_page_bitmap[byte_idx] |= bit_set_mask;
        return _page;
    }
    return 0;
}
uint32_t
get_base_page(void)
{
    uint32_t target_page = get_base_page_fast();
    if (!target_page)
        target_page = get_base_page_slow();
    return target_page;
}
/*
 * release a page which must be in PageInventory VMA.
 */
void
free_base_page(uint32_t _page)
{
    int32_t offset;
    int32_t byte_idx = 0;
    int32_t bit_idx = 0;
    uint8_t bit_set_mask = 0;
    uint8_t bit_unset_mask = 0;
    if (_page < PAGE_SPACE_BOTTOM ||
        _page >= PAGE_SPACE_TOP) {
        LOG_WARN("seem page:0x%x is not in PageInventory VMA\n");
        return; 
    }
    offset = _page - PAGE_SPACE_BOTTOM;
    offset = offset >> 12;
    byte_idx = offset >> 3;
    bit_idx = offset & 0x7;
    bit_set_mask = 1 << bit_idx;
    bit_unset_mask = ~bit_set_mask;
    free_base_page_bitmap[byte_idx] &= bit_unset_mask;
}
/*
 * map phy_addr to virt_addr in kernel linear address space
 * if page table is not present for a page directory entry
 * allocate one page from physical page inventory
 */
void
kernel_map_page(uint32_t virt_addr,
    uint32_t phy_addr,
    uint32_t write_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable)
{
    uint32_t page_table = 0;
    uint32_t * page_table_ptr;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    struct pde32 * pde = PDE32_PTR(&kernel_page_directory[pd_index]);
    struct pte32 * pte;
    if(!pde->present) {
        page_table = get_base_page();
        ASSERT(page_table);
        /*
         * MY GOD, the page should be clear once allocated
         * or the table will be polluted initially.
         */
        memset((void *)page_table, 0x0, PAGE_SIZE);
        kernel_page_directory[pd_index] = create_pde32(
            PAGE_PERMISSION_READ_WRITE,
            PAGE_PERMISSION_SUPERVISOR,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED,
            page_table);
        LOG_DEBUG("allocate page table for page directory index:%x\n", pd_index);
        pde = PDE32_PTR(&kernel_page_directory[pd_index]);
    }
    ASSERT(pde->present);
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    page_table_ptr[pt_index] = create_pte32(write_permission,
        PAGE_PERMISSION_SUPERVISOR,
        page_writethrough,
        page_cachedisable,
        phy_addr);
    pte = PTE32_PTR(&page_table_ptr[pt_index]);
    ASSERT(pte->present);
    ASSERT((pte->pg_frame << 12) == (phy_addr & (~PAGE_MASK)));
    //LOG_INFO("map %x to %x %x\n", phy_addr, virt_addr, page_table_ptr[pt_index]);
}
/*
 * allocate physically continuous pages
 * return the address of the 1st page
 */
uint32_t
get_pages(int nr_pages)
{
    //addr is page aligned
    uint32_t addr = get_system_memory_start();
    uint32_t boundary = get_system_memory_boundary();
    int idx = 0;
    int nstep = 1;
    int match = 0;
    ASSERT(!(addr & PAGE_MASK));
    for (; addr < boundary; addr += PAGE_SIZE * nstep) {
        if ((addr + PAGE_SIZE * nr_pages) >= boundary)
            break;
        for(idx = 0; idx < nr_pages; idx++){
            if (!IS_PAGE_FREE(addr + idx * PAGE_SIZE))
                break;
        }
        if (idx == nr_pages) {
            match = 1;
            break;
        }
        nstep = idx + 1;
    }
    if (match) {
        for (idx = 0; idx < nr_pages; idx++) {
            ASSERT(IS_PAGE_FREE(addr + idx * PAGE_SIZE));
            MARK_PAGE_AS_OCCUPIED(addr + idx * PAGE_SIZE);
            ASSERT(!IS_PAGE_FREE(addr + idx * PAGE_SIZE));
        }
        return addr;
    }
    return 0;
}
/*
 * It's crutial to import fast free page searching method, it save lots of time
 * and it almost approches o(1) time.
 */
static uint32_t fast_page_addr = 0;
uint32_t
get_page_fast(void)
{
    uint32_t target_page = 0;
    uint32_t addr = get_system_memory_start();
    uint32_t boundary = get_system_memory_boundary();
    if (fast_page_addr < addr || fast_page_addr >= boundary)
        fast_page_addr = addr;
    for (; fast_page_addr < boundary; fast_page_addr += PAGE_SIZE) {
        if(IS_PAGE_FREE(fast_page_addr)) {
            target_page = fast_page_addr;
            MARK_PAGE_AS_OCCUPIED(fast_page_addr);
            ASSERT(!IS_PAGE_FREE(fast_page_addr));
            fast_page_addr += PAGE_SIZE;
            break;
        }
    }
    return target_page;
}

uint32_t
get_page(void)
{
    uint32_t _page = get_page_fast();
    if (!_page)
        _page = get_pages(1);
    return _page;
}

void
free_pages(uint32_t pg_addr, int nr_pages)
{
    int idx = 0;
    for(; idx < nr_pages; idx++){
        MAKR_PAGE_AS_FREE(pg_addr + PAGE_SIZE * idx);
    }
}

void
free_page(uint32_t pg_addr)
{
    free_pages(pg_addr, 1);
}

void
enable_paging(void)
{
    asm volatile("movl %%cr0, %%eax;"
        "or $0x80000000, %%eax;"
        "movl %%eax, %%cr0;"
        :
        :
        :"%eax");
}

void
disable_paging(void)
{
    asm volatile("movl %%cr0, %%eax;"
        "and $0x7fffffff, %%eax;"
        "movl %%eax, %%cr0;"
        :
        :
        :"%eax");
}

void
flush_tlb(void)
{
    asm volatile("movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        :
        :"%eax", "memory");
}
uint32_t
get_kernel_page_directory(void)
{
    return (uint32_t)kernel_page_directory;
}
void
dump_page_tables(uint32_t page_directory)
{
    uint32_t * pd_ptr = (uint32_t *)page_directory;
    int pd_index = 0;
    int pg_index = 0;
    struct pde32 * pde;
    struct pte32 * pte;
    uint32_t virtual_addr;

    LOG_INFO("dump page table:\n");
    for(pd_index = 0; pd_index < 1024; pd_index++) {
        pde = PDE32_PTR(&pd_ptr[pd_index]);
        if(!pde->present)
            continue;
        printk("    page directory entry:%d 0x%x\n",
            pd_index,
            pd_ptr[pd_index]);
        for(pg_index = 0; pg_index < 1024; pg_index++) {
            pte = PTE32_PTR(((pde->pt_frame << 12) + pg_index * 4));
            if (!pte->present)
                continue;
            virtual_addr = pd_index << 22 | pg_index << 12;
            printk("        page table entry:%d (virt:%x) 0x%x\n",
                pg_index,
                virtual_addr,
                PTE32_TO_DWORD(pte));
        }
    }
}
/*
 * Convert to virtual/linear address into physical address by search the
 * page directory/tables.
 */
__attribute__((always_inline)) inline uint32_t
virt2phy(uint32_t * page_directory, uint32_t virt_addr)
{
#define _(con) if(!(con)) goto out;
    uint32_t phy_addr = 0;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    uint32_t * page_table_ptr = NULL;
    struct pde32 * pde = NULL;
    struct pte32 * pte = NULL;
    pde = PDE32_PTR(&page_directory[pd_index]);
    _(pde->present);
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    pte = PTE32_PTR(&page_table_ptr[pt_index]);
    _(pte->present);
    phy_addr = (((uint32_t)pte->pg_frame) << 12);
    phy_addr |= virt_addr & PAGE_MASK;
    out:
    return phy_addr;
#undef _
}

/*
 * Test whether the page mapping is present.
 * return OK if the virtual address is mapped.
 * any non-OK value returned indicate the page is non-present
 */
uint32_t
page_present(uint32_t * page_directory, uint32_t virt_addr)
{
#define _(con) if(!(con)) goto out;
    uint32_t present = -ERR_NOT_PRESENT;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    uint32_t * page_table_ptr = NULL;
    struct pde32 * pde = NULL;
    struct pte32 * pte = NULL;
    pde = PDE32_PTR(&page_directory[pd_index]);
    _(pde->present);
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    pte = PTE32_PTR(&page_table_ptr[pt_index]);
    _(pte->present);
    present = OK;
    out:
        return present;
#undef _
}

void
enable_kernel_paging(void)
{
    int idx = 0;
    uint32_t directory_index_top = USERSPACE_BOTTOM >> 12 >> 10;
    uint32_t * kernel_page_directory = (uint32_t *)get_kernel_page_directory();
    for(idx = directory_index_top; idx < 1024; idx++)
        kernel_page_directory[idx] = 0x0;
    __asm__ volatile("movl %%eax, %%cr3;"
        :
        :"a"(kernel_page_directory));
}
void
paging_init(void)
{
    int idx = 0;
    uint32_t phy_addr = 0;
    uint32_t frame_addr = 0;
    uint32_t sys_mem_start = get_system_memory_start();
    uint32_t sys_mem_boundary = get_system_memory_boundary();
    memset(free_page_bitmap, 0x0, sizeof(free_page_bitmap));
    memset(free_base_page_bitmap, 0x0, sizeof(free_base_page_bitmap));
    /*
     * Marks pages above system memory boundary as occupised
     *
     */
    for(idx = AT_BYTE(sys_mem_boundary >> 12);
        idx < FREE_PAGE_BITMAP_SIZE;
        idx++) {
        free_page_bitmap[idx] = 0xff;
    }
    /*
     * Mark pages whose address is lower than the _kernel_bss_end as occupied
     */
    for (frame_addr = 0;
        frame_addr < (uint32_t)&_kernel_bss_end;
        frame_addr += PAGE_SIZE) {
        MARK_PAGE_AS_OCCUPIED(frame_addr);
        ASSERT(!IS_PAGE_FREE(frame_addr));
    }
    /*
     * Allocate the kernel page directory
     * */
    kernel_page_directory = (uint32_t *)get_base_page();
    memset(kernel_page_directory, 0x0, PAGE_SIZE);
    LOG_INFO("kernel page directory address: 0x%x\n", kernel_page_directory);
    /*
     *Mark pages in page inventory as occupied
     *and map them in advance
     */
    for(frame_addr = PAGE_SPACE_BOTTOM;
        frame_addr < PAGE_SPACE_TOP;
        frame_addr += PAGE_SIZE) {
        MARK_PAGE_AS_OCCUPIED(frame_addr);
        ASSERT(!IS_PAGE_FREE(frame_addr));
    }
    for(frame_addr = PAGE_SPACE_BOTTOM;
        frame_addr < PAGE_SPACE_TOP;
        frame_addr += PAGE_SIZE) {
        kernel_map_page(frame_addr,
            frame_addr,
            PAGE_PERMISSION_READ_WRITE,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED);
    }
    /*
     * Map all the pages before the returned address of 
     * get_system_memory_start()
     */
    for(phy_addr = 0; phy_addr < sys_mem_start; phy_addr += PAGE_SIZE) {
        kernel_map_page(phy_addr, phy_addr,
            (phy_addr >= (uint32_t)&_kernel_text_start &&
            phy_addr < (uint32_t)&_kernel_data_start) ?
            PAGE_PERMISSION_READ_ONLY :
            PAGE_PERMISSION_READ_WRITE,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED);
    }
    
    //dump_page_tables((uint32_t)kernel_page_directory);
    /*
     * Enable paging
     */
    asm volatile("movl %%eax, %%cr3;"
        "movl %%cr0, %%eax;"
        "or $0x80010001, %%eax;"
        "movl %%eax, %%cr0;"
        :
        :"a"((uint32_t)kernel_page_directory));
}
