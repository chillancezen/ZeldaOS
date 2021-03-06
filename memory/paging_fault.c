/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <kernel/include/printk.h>
#include <x86/include/interrupt.h>
#include <kernel/include/task.h>

#define PAGING_FAULT_INTERRUPT_VECTOR 14

extern uint32_t return_from_pl3_signal_context(uint32_t * p_esp);

static uint32_t
handle_kernel_page_fault(struct x86_cpustate * cpu,
    uint32_t linear_addr,
    uint32_t * p_esp)
{
    uint32_t error_code = cpu->errorcode;
    struct kernel_vma * vma;
    uint32_t phy_addr = 0;
    ASSERT(linear_addr < ((uint32_t)USERSPACE_BOTTOM));
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
                linear_addr - vma->virt_addr + vma->phy_addr:
                get_page();
            /*
             * physical address is not supposed to be 0x0
             * because it's already mapped as Low1MB area
             */
            ASSERT(phy_addr);
            kernel_map_page(linear_addr, phy_addr,
                vma->write_permission,
                vma->page_writethrough,
                vma->page_cachedisable);
            LOG_TRIVIA("Map kernel page in VMA:%s 0x%x --> 0x%x\n",
                vma->name,
                linear_addr,
                phy_addr);
        } else {
            LOG_ERROR("no VMA found for addr:0x%x\n", linear_addr);
            dump_x86_cpustate(cpu);
            __not_reach();
        }

    } else {
        /*
         * At the preparation phaze of PL3 signal context, we fill the PL3
         * stack with the return addess as `return_from_pl3_signal_context`
         * , but this function resides in kernel land, the process will
         * generate paging permission fault, we catch it here.
         */
        if (linear_addr == (uint32_t)return_from_pl3_signal_context) {
            return return_from_pl3_signal_context(p_esp);
        }
        LOG_ERROR("Paging permission violation, task:0x%x linear_addr:0x%x\n",
            current, linear_addr);
        dump_x86_cpustate(cpu);
        if (current) {
            signal_task(current, SIGKILL);
        } else {
            hlt();
        }
    }
    return OK;
}

static uint32_t
paging_fault_handler(struct x86_cpustate * cpu)
{
    uint32_t result = OK;
    uint32_t esp = (uint32_t)cpu;
    uint32_t linear_addr;
    asm volatile("movl %%cr2, %%edx;"
        "movl %%edx, %0;"
        :"=m"(linear_addr)
        :
        :"%edx");
    LOG_TRIVIA("paging fault {linear address:0x%x, task:0x%x}\n",
        linear_addr, current);
    if (linear_addr < ((uint32_t)USERSPACE_BOTTOM)) {
        result = handle_kernel_page_fault(cpu, linear_addr, &esp);
        if (result == OK) {
            if (current)
                enable_task_paging(current);
            else
                enable_kernel_paging();
        }
    } else {
        ASSERT(current);
        result = handle_userspace_page_fault(current, cpu, linear_addr);
        if (result == OK) {
            if (current)
                enable_task_paging(current);
            else
                enable_kernel_paging();
        }
    }
    /*
     * Page Fault is not occuring as expected.
     */
    if (result != OK) {
        LOG_DEBUG("linear address:0x%x error code:0x%x\n",
            linear_addr, cpu->errorcode);
        dump_x86_cpustate(cpu);
        __not_reach();
    }
    return esp;
}

void
paging_fault_init(void)
{
    register_interrupt_handler(PAGING_FAULT_INTERRUPT_VECTOR,
        paging_fault_handler,
        "Paging Fault Handler");
}
