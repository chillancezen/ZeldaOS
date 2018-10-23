/*
 * Copyright (c) 2018 Jie Zheng
 */


#include <kernel/include/task.h>
#include <kernel/include/system_call.h>
#include <kernel/include/zelda_posix.h>

#define CPU_YIELD_TRAP_VECTOR 0x88

void
yield_cpu(void)
{
    asm volatile("int %0;"
        :
        :"i"(CPU_YIELD_TRAP_VECTOR));
}
static uint32_t
cpu_yield_handler(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;
    if (ready_to_schedule()) {
        esp = schedule(cpu);
    }
    return esp;
}

static int32_t
call_sys_exit(struct x86_cpustate * cpu, uint32_t exit_code)
{
    ASSERT(current);
    current->state = TASK_STATE_EXITING;
    current->exit_code = exit_code;
    yield_cpu();
    return OK;
}


void
task_misc_init(void)
{
    register_interrupt_handler(CPU_YIELD_TRAP_VECTOR,
        cpu_yield_handler,
        "CPU Yield Trap");
    register_system_call(SYS_EXIT_IDX, 1, (call_ptr)call_sys_exit);
}
