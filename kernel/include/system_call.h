/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _SYSTEM_CALL_H
#define _SYSTEM_CALL_H
#include <lib/include/types.h>
#include <x86/include/interrupt.h>

void system_call_init(void);

#define SYSCALL_NUM(cpu) (cpu)->eax

#define SYSCALL_RETURN(cpu) (cpu)->eax

#define SYSCALL_ARG0(cpu) (cpu)->ebx
#define SYSCALL_ARG1(cpu) (cpu)->ecx
#define SYSCALL_ARG2(cpu) (cpu)->edx
#define SYSCALL_ARG3(cpu) (cpu)->esi
#define SYSCALL_ARG4(cpu) (cpu)->edi


#define MAX_SYSCALL_NUM 255

typedef int32_t (*call_ptr)(struct x86_cpustate * cpu, ...);

void register_system_call(int call_num,
    int nr_args,
    call_ptr call);



#endif
