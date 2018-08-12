/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <kernel/include/printk.h>
#include <x86/include/interrupt.h>

#define PAGING_FAULT_INTERRUPT_VECTOR 14


static void
do_kernel_page_fault(uint32_t error_code, uint32_t linear_addr)
{
    struct kernel_vma * vma;
    uint32_t phy_addr = 0;;
    if ((error_code & 0x1) == 0x0) {
        /*
         * the 0th bit of errorcode indicates whether the exception is caused
         * by accessing to the no-mapped virtual address, if the bit is not
         * set, the page is not present, here we search VMA array to determine
         * where to map the new page
         */
        vma = search_kernel_vma(linear_addr);
        if (vma) {
            phy_addr = vma->exact ?
                vma->phy_addr + vma->virt_addr - linear_addr:
                get_page();
            /*
             * physical address is not supposed to be 0x0
             * because it's already mapped as Low1MB area
             */ 
            ASSERT(phy_addr);
            kernel_map_page(linear_addr, phy_addr, PAGE_PERMISSION_READ_WRITE);
        } else {
            LOG_ERROR("no VMA found for addr:0x%x\n", linear_addr);
        }

    }
}

static void
paging_fault_handler(struct x86_cpustate * pt_regs)
{
    uint32_t linear_addr;
    asm volatile("movl %%cr2, %%edx;"
        "movl %%edx, %0;"
        :"=m"(linear_addr)
        :
        :"%edx");
    disable_paging();
    do_kernel_page_fault(pt_regs->errorcode, linear_addr);
    //flush_tlb();
    enable_paging();
}

void
paging_fault_init(void)
{
    register_interrupt_handler(PAGING_FAULT_INTERRUPT_VECTOR,
        paging_fault_handler,
        "Paging Fault Handler");
}
