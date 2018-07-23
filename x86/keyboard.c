/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/keyboard.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>
#include <x86/include/ioport.h>

#define KEYBOARD_INTERRUPT_VECTOR (0x20 + 1)

void __switch_led(uint8_t mask)
{
    outb(KEYBOARD_COMMAND_PORT, 0xed);
    __asm__ volatile("pause;");
    outb(KEYBOARD_COMMAND_PORT, mask);
}

void keyboard_interrupt_handler(struct interrup_argument * parg __used)
{
    uint8_t scancode = inb(0x60);
    printk("keyboard interrupted:%x\n", scancode); 
}


void
keyboard_init(void)
{
    register_interrupt_handler(KEYBOARD_INTERRUPT_VECTOR,
        keyboard_interrupt_handler,
        "PS/2 keyboard");
}
