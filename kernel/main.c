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
#include <memory/include/multiboot_mmap.h>

void kernel_main(struct multiboot_info * boot_info __used, void * magicnum __used)
{
    printk_init();
    gdt_init();
    idt_init();
    pit_init();
    keyboard_init();
    probe_physical_mmeory(boot_info);
    sti();
#if 0
    //enter static tss0
    asm volatile("jmpl %0, $0x0;"
        :
        :"i"(TSS0_SELECTOR));
#endif
}
