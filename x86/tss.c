/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/tss.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <x86/include/gdt.h>

struct task_state_segment static_tss0 __attribute__((aligned(4096)));
uint8_t tss0_stack[8192] __attribute__((aligned(4)));
uint8_t tss0_kernel_stack[8192] __attribute__((aligned(4)));


void __initialize_tss(struct task_state_segment * tss,
    uint16_t code_selector,
    uint16_t data_selector,
    void (*entry)(void))
{
    memset(tss, 0x0, sizeof(struct task_state_segment));

    tss->ds = data_selector;
    tss->es = data_selector;
    tss->fs = data_selector;
    tss->gs = data_selector;
    tss->ss = data_selector;
    tss->cs = code_selector;
    tss->esp = (uint32_t)tss0_stack + sizeof(tss0_stack);
    tss->ss0 = KERNEL_DATA_SELECTOR;
    tss->esp0 = (uint32_t)tss0_kernel_stack + sizeof(tss0_kernel_stack);

    tss->eip = (uint32_t)entry;
    tss->eflags = 0;
}

static void
tss0_entry(void)
{
    //while(1) {
    //    dump_registers();
    //}
    printk("hello TSS utilities\n");
}

void tss_init(void)
{
    __initialize_tss(&static_tss0,
        USER_CODE_SELECTOR,
        USER_DATA_SELECTOR,
        tss0_entry);
}
