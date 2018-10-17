/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/tss.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <x86/include/gdt.h>

struct task_state_segment static_tss0 __attribute__((aligned(4096)));
uint8_t tss0_stack[8192] __attribute__((aligned(4)));
uint8_t tss0_kernel_stack[1024*1024*4] __attribute__((aligned(4)));


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
    LOG_DEBUG("static tss privilege level 0 stack top:0x%x\n", tss->esp0);
    tss->eip = (uint32_t)entry;
    tss->eflags = 0;
}

/*
 * Set the ss0:esp0 of the static tss. thus when a PL3 task is interruped
 * , the task switching occurs and a right PL0 stack can be found and serve the
 * trapped services.
 */
void
set_tss_privilege_level0_stack(uint32_t esp0)
{
    memset(&static_tss0, 0x0, sizeof(static_tss0));
    static_tss0.ss0 = KERNEL_DATA_SELECTOR;
    static_tss0.esp0 = esp0;
} 
static void
tss0_entry(void)
{
    ASSERT(0);
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
