/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/timer.h>
#include <x86/include/ioport.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>

#define TIMER_RESOLUTION_HZ 1
#define PIT_CHANNEL0_INTERRUPT_VECTOR (0x20 + 0)

static void
refresh_pit_channel0(void)
{
    int divisor = OSCILLATPR_CHIP_FREQUENCY / TIMER_RESOLUTION_HZ;
    outb(PIT_CONTROL_PORT, 0x6 | 0x30);
    outb(PIT_CHANNEL0_PORT, divisor & 0xff);
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xff);
}

static void
pit_handler(struct interrup_argument * pit __used)
{
    //printk("pit interrupted\n");
}

void
pit_init(void)
{
    register_interrupt_handler(PIT_CHANNEL0_INTERRUPT_VECTOR, pit_handler);
    refresh_pit_channel0();
}


