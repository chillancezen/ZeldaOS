/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/interrupt.h>
#include <kernel/include/task.h>
#include <kernel/include/printk.h>

#define GENERAL_PROTEXTION_FAULT_VECTOR 13

static uint32_t
protection_fault_handler(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;
    printk("GP: current:%x\n", current);
    dump_x86_cpustate(cpu);
    dump_x86_cpustate(current->cpu);
    hlt();
    return esp;
}

void
gp_init(void)
{
    register_interrupt_handler(GENERAL_PROTEXTION_FAULT_VECTOR,
        protection_fault_handler,
        "Geneal Protection Fault");
}
