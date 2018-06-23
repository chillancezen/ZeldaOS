/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>
#include <x86/include/timer.h>

void kernel_main(void * multiboot, void * magicnum)
{
    printk_init();
    multiboot = magicnum;
    magicnum = multiboot;
    gdt_init();
    idt_init();
    pit_init();
    __asm__ volatile("sti;");
#if 0
    asm ("int $0x80");
    asm ("int $0x81");
#endif
}
