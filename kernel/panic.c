/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/kernel.h>
#include <kernel/include/printk.h>
#include <memory/include/physical_memory.h>

void
dump_registers(void)
{
    uint32_t CR0 = 0;// controll register 0
    uint32_t CR3 = 0;// controll register 3
    uint32_t CS = 0;
    uint32_t DS = 0;
    uint32_t SS = 0;
    uint32_t ES = 0;
    uint16_t TR = 0; // Task Register
    __asm__ volatile("mov %%cs, %%dx;"
        "mov %%dx, %[CS];"
        "mov %%ds, %%dx;"
        "mov %%dx, %[DS];"
        "mov %%ss, %%dx;"
        "mov %%dx, %[SS];"
        "mov %%es, %%dx;"
        "mov %%dx, %[ES];"
        "mov %%CR0, %%edx;"
        "mov %%edx, %[CR0];"
        "mov %%CR3, %%edx;"
        "mov %%edx, %[CR3];"
        :[CS]"=m"(CS), [DS]"=m"(DS), [SS]"=m"(SS),[ES]"=m"(ES),
        [CR0]"=m"(CR0), [CR3]"=m"(CR3)
        :
        :"%edx");
    __asm__ volatile("str %0;"
        :"=m"(TR)
        :
        :"memory");
    LOG_INFO("tr:%x,cr0:%x cr3:%x cs:%x ds:%x ss:%x es:%x\n",
        TR, CR0, CR3, CS, DS, SS, ES);
}

void
panic(void)
{
    int32_t frame = 0x0;
    uint32_t ebp;
    uint32_t eip;
    asm volatile("movl %%ebp, %%edx;"
        "movl %%edx, %[ebp];"
        :[ebp]"=m"(ebp)
        :
        :"%edx");
    LOG_INFO("Dump the calling stack with initial ebp(esp): 0x%x\n", ebp);
    LOG_INFO("(actually the next instruction is given, "
        "please refer to previous instruction)\n");
    for (; frame < MAX_FRAME_ON_DUMPSTACK; frame++) {
        eip = ((uint32_t *)ebp)[1];
        if (eip < (uint32_t)&_kernel_text_start ||
            eip > (uint32_t)&_kernel_text_end)
            break;
        LOG_INFO("   frame #%d <%x>\n", frame, eip);
        ebp = ((uint32_t *)ebp)[0];
        //XXX: another bug fix: do not jump to user land in case the kernel
        //in a interrupt context.
        if (ebp >= ((uint32_t)USERSPACE_BOTTOM))
            break;
    }
}
