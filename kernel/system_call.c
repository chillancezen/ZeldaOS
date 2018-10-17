/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/system_call.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>

#define SYSTEM_CALL_TRAP_VECTOR 0x87

struct syscall_entry {
    int valid;
    int nr_args;
    int32_t (*call)(struct x86_cpustate * cpu, ...);
};

static struct syscall_entry syscall_entries[MAX_SYSCALL_NUM];

static uint32_t
system_call_handler(struct x86_cpustate * cpu)
{
    int32_t syscall_ret = -ERR_GENERIC;
    uint32_t esp = (uint32_t)cpu;
    int32_t syscall_num = SYSCALL_NUM(cpu);
    printk("syscall %x\n", SYSCALL_NUM(cpu));
    if (syscall_num < 0 || syscall_num >= MAX_SYSCALL_NUM)
        goto out;
    if (!syscall_entries[syscall_num].valid)
        goto out;
    switch (syscall_entries[syscall_num].nr_args)
    {
        case 0:
            syscall_ret = syscall_entries[syscall_num].call(cpu);
            break;
        case 1:
            syscall_ret = syscall_entries[syscall_num].call(cpu,
                SYSCALL_ARG0(cpu));
            break;
        case 2:
            syscall_ret = syscall_entries[syscall_num].call(cpu,
                SYSCALL_ARG0(cpu),
                SYSCALL_ARG1(cpu));
            break;
        case 3:
            syscall_ret = syscall_entries[syscall_num].call(cpu,
                SYSCALL_ARG0(cpu),
                SYSCALL_ARG1(cpu),
                SYSCALL_ARG2(cpu));
            break;
        case 4:
            syscall_ret = syscall_entries[syscall_num].call(cpu,
                SYSCALL_ARG0(cpu),
                SYSCALL_ARG1(cpu),
                SYSCALL_ARG2(cpu),
                SYSCALL_ARG3(cpu));
            break;
        case 5:
            syscall_ret = syscall_entries[syscall_num].call(cpu,
                SYSCALL_ARG0(cpu),
                SYSCALL_ARG1(cpu),
                SYSCALL_ARG2(cpu),
                SYSCALL_ARG3(cpu),
                SYSCALL_ARG4(cpu));
            break;
        default:
            LOG_ERROR("Unsupported syscall argument number\n");
            break;      
    }
    out:
        SYSCALL_RETURN(cpu) = syscall_ret;
        return esp;
}
void register_system_call(int call_num,
    int nr_args,
    call_ptr  call)
{
    syscall_entries[call_num].valid = 1;
    syscall_entries[call_num].nr_args = nr_args;
    syscall_entries[call_num].call = call;
    LOG_INFO("Register system call entry(index:%d nr_args:%d entry:0x%x)\n",
        call_num,
        nr_args,
        call);
}

void
system_call_init(void)
{
    register_interrupt_handler(SYSTEM_CALL_TRAP_VECTOR,
        system_call_handler,
        "System Call Trap");
    memset(syscall_entries, 0x0, sizeof(syscall_entries));
}
