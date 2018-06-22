/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/ioport.h>

inline uint8_t
inb(uint16_t portid)
{
    uint8_t ret;
    asm volatile("inb %%dx;"
        :"=a"(ret)
        :"d"(portid));
    return ret;
}

inline
void outb(uint16_t portid, uint8_t val)
{
    asm volatile("outb %%dx;"
        :
        :"a"(val), "d"(portid));
}
inline void outb_slow(uint16_t portid, uint8_t val)
{
    asm volatile("outb %%dx;"
        "jmp 1f;"
        "1:jmp 1f;"
        "1:"
        :
        :"a"(val), "d"(portid));
}
