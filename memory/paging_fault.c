/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/paging.h>
#include <kernel/include/printk.h>
#include <x86/include/interrupt.h>

#define PAGING_FAULT_INTERRUPT_VECTOR 14
static void
paging_fault_handler(struct interrupt_argument * pt_regs)
{
    uint32_t cr2;
    asm volatile("movl %%cr2, %%edx;"
        "movl %%edx, %0;"
        :"=m"(cr2)
        :
        :"%edx");
    printk("esp: %x %x\n", pt_regs->errorcode, cr2);
}

void
paging_fault_init(void)
{
    register_interrupt_handler(PAGING_FAULT_INTERRUPT_VECTOR,
        paging_fault_handler,
        "Paging Fault Handler");
}
