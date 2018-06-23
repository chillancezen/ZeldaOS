/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/timer.h>
#include <x86/include/ioport.h>

#define TIMER_RESOLUTION_HZ 10

void
pit_init(void)
{
    int divisor = OSCILLATPR_CHIP_FREQUENCY / TIMER_RESOLUTION_HZ;
    outb(PIT_CONTROL_PORT, 0x6 | 0x30);
    outb(PIT_CHANNEL0_PORT, divisor & 0xff);
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xff);
}
