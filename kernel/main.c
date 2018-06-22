/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>

void kernel_main(void * multiboot, void * magicnum)
{
    int a = 0;
    printk_init();
    multiboot = magicnum;
    magicnum = multiboot;
    gdt_init();
    idt_init();
    __asm__ volatile("sti;");
    
    printk("magic:%x\n", a / a);
}
