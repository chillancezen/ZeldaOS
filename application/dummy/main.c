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


void
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

void
print_hexdecimal(uint32_t val)
{
    int idx = 0;
    uint8_t dict[] = "0123456789abcdef";
    uint8_t * ptr = (uint8_t *)&val;

    for(idx = 0, ptr += 3; idx < 4; idx++, ptr--) {
        write_serial(dict[((*ptr) >> 4) & 0xf]);
        write_serial(dict[*ptr & 0xf]);
    }
}
extern void * _zelda_constructor_init_start;
extern void * _zelda_constructor_init_end;

static void
interrupt_handler(int signal)
{
    print_serial("Application Signal interrupted\n");
    sleep(100);
    print_serial("Application Signal interrupted ends\n");
    //kill(-1, SIGKILL);
}
int main(int argc, char *argv[])
{
    int32_t ret = 0;
    ret = signal(SIGINT, interrupt_handler);
    while(0) {
        print_serial(argv[0]);
        print_serial("\n");
        sleep(10000);
        //kill(-1, SIGINT);
    }
    if (1) {
        int fd = open("/home/foo", O_RDWR|O_CREAT);
        print_serial("opened fd:");
        print_hexdecimal(fd);
        print_serial("\n");
    }
#if 0
    *(uint32_t *)0x42802000 = 0;
    int _start = (int)&_zelda_constructor_init_start;
    int _end = (int)&_zelda_constructor_init_end;
    int addr = 0;
    int idx = 0;
    print_serial("hello Application\n");
    for(addr = _start; addr < _end; addr += 4) {
        ((void (*)(void))*(int*)addr)();
    }
    for(idx = 0; idx < argc; idx++) {
        print_hexdecimal(argv[idx]);
        print_serial("\n");
    }
    print_serial("End of Application\n");
    *(uint32_t *)0x507 = 0;
#endif
    return 0;
}
