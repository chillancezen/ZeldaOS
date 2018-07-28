/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>


static uint8_t free_page_bitmap[FREE_PAGE_BITMAP_SIZE];
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
 * map phy_addr to virt_addr in kernel linear address space
 * if page table is not present for a page directory entry
 * allocate one page from physical page inventory
 */
void
kernel_map_page(uint32_t virt_addr,
    uint32_t phy_addr,
    uint32_t write_permission)
{
    uint32_t page_table = 0;
    uint32_t * page_table_ptr;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    struct pde32 * pde = PDE32_PTR(&kernel_page_directory[pd_index]);
    struct pte32 * pte;
    if(!pde->present) {
        page_table = get_page();
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
        LOG_INFO("allocate page table for page directory index:%x\n", pd_index);
        pde = PDE32_PTR(&kernel_page_directory[pd_index]);
    }
    ASSERT(pde->present);
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    page_table_ptr[pt_index] = create_pte32(write_permission,
        PAGE_PERMISSION_SUPERVISOR,
        PAGE_WRITEBACK,
        PAGE_CACHE_ENABLED,
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

uint32_t
get_page(void)
{
    return get_pages(1);
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
    printk("flush TLB\n");
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

void
paging_init(void)
{
    uint32_t phy_addr = 0;
    uint32_t frame_addr = 0;
    uint32_t sys_mem_start = get_system_memory_start();
    memset(free_page_bitmap, 0x0, sizeof(free_page_bitmap));
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
    kernel_page_directory = (uint32_t *)get_page();
    memset(kernel_page_directory, 0x0, PAGE_SIZE);
    LOG_INFO("kernel page directory address: 0x%x\n", kernel_page_directory);
    /*
     * Map all the pages before the returned address of 
     * get_system_memory_start()
     */
    for(phy_addr = 0; phy_addr < sys_mem_start; phy_addr += PAGE_SIZE) {
        kernel_map_page(phy_addr, phy_addr,
            (phy_addr >= (uint32_t)&_kernel_text_start &&
            phy_addr < (uint32_t)&_kernel_data_start) ?
            PAGE_PERMISSION_READ_ONLY :
            PAGE_PERMISSION_READ_WRITE);
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
    
    //*(uint32_t*)paging_init = 0;
    //printk("%x\n", kernel_page_directory[0]);
}
