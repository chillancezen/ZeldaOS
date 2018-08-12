/*
 * Copyright (c) 2018 Jie Zheng
 * the Timer framework will employ HEAP sort algorithm to determine expired
 * timer instance quickly: https://github.com/chillancezen/scalable-timer
 */

#include <x86/include/timer.h>
#include <x86/include/ioport.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>


#define TIMER_RESOLUTION_HZ 100 //10 ms tick
#define PIT_CHANNEL0_INTERRUPT_VECTOR (0x20 + 0)

static uint32_t pit_ticks = 0;

static void
refresh_pit_channel0(void)
{
    int divisor = OSCILLATPR_CHIP_FREQUENCY / TIMER_RESOLUTION_HZ;
    outb(PIT_CONTROL_PORT, 0x6 | 0x30);
    outb(PIT_CHANNEL0_PORT, divisor & 0xff);
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xff);
}

static void
pit_handler(struct x86_cpustate * pit __used)
{
    pit_ticks++;
    if(pit_ticks % TIMER_RESOLUTION_HZ == 0) {
        //printk("pit interrupted\n");
    }
}

void
pit_init(void)
{
    register_interrupt_handler(PIT_CHANNEL0_INTERRUPT_VECTOR,
        pit_handler,
        "Intel 8253 PIT");
    refresh_pit_channel0();
}


