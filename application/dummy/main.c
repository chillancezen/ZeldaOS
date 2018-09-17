/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>

#define asm __asm__
int cute = 1;
int bar[1024*1024*10];
#define COM1_PORT 0x3f8

uint8_t
inb(uint16_t portid)
{
    uint8_t ret;
    asm volatile("inb %1, %0;"
        :"=a"(ret)
        :"Nd"(portid));
    return ret;
}

void
outb(uint16_t portid, uint8_t val)
{
    asm volatile("outb %%al, %%dx;"
        :
        :"a"(val), "Nd"(portid));
}


int
is_transmit_empty(void)
{
    return inb(COM1_PORT + 5) & 0x20;
}

void
write_serial(uint8_t a)
{
    while (is_transmit_empty() == 0);
    outb(COM1_PORT,a);
}


__attribute__((constructor)) void
foo(void)
{
    __asm__ volatile("movl $0x1, %%eax;"
        "int $0x87;"
        :
        :
        :"%eax");
}
void
print_serial(char * str)
{
     while(*str)
         write_serial(*str++);
}
extern void * _zelda_constructor_init_start;
extern void * _zelda_constructor_init_end;

int main(void)
{
    int _start = (int)&_zelda_constructor_init_start;
    int _end = (int)&_zelda_constructor_init_end;
    int addr = 0;
    print_serial("hello Application\n");
    for(addr = _start; addr < _end; addr += 4) {
        ((void (*)(void))*(int*)addr)();
    }
    print_serial("End of Application\n");
    return 0;
}
