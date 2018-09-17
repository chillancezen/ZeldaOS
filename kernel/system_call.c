/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/system_call.h>

#define SYSTEM_CALL_TRAP_VECTOR 0x87

static uint32_t
system_call_handler(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;

    return esp;
}

void
system_call_init(void)
{
    register_interrupt_handler(SYSTEM_CALL_TRAP_VECTOR,
        system_call_handler,
        "System Call Trap");
}
