/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>
#include <x86/include/timer.h>
#include <x86/include/keyboard.h>

void kernel_main(void * multiboot __used, void * magicnum __used)
{
    printk_init();
    gdt_init();
    idt_init();
    pit_init();
    keyboard_init();
    sti();
}
