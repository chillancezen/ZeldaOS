/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>
#include <x86/include/timer.h>
#include <device/include/keyboard.h>
#include <memory/include/physical_memory.h>
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <device/include/serial.h>
#include <lib/include/string.h>

static struct multiboot_info * boot_info;

static void
init1(void)
{
    serial_init();
    printk_init();
    gdt_init();
    idt_init();
}

static void
init2(void)
{
    probe_physical_mmeory(boot_info);
    paging_init();
    paging_fault_init();
    kernel_vma_init();
}
static void
init3(void)
{
    pit_init();
    keyboard_init();
}
void kernel_main(struct multiboot_info * _boot_info, void * magicnum __used)
{
    boot_info = _boot_info;
    init1();
    init2();
    init3();
    sti();
#if 0
    //enter static tss0
    asm volatile("jmpl %0, $0x0;"
        :
        :"i"(TSS0_SELECTOR));
#endif
    //*(uint32_t*)kernel_main = 0;
}
