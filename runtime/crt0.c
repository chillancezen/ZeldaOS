/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

extern void * _zelda_constructor_init_start;
extern void * _zelda_constructor_init_end;
#define PSEUDO_TERMINAL_KEY "tty"
#define CWD_ENV_KEY         "cwd"
#define DEFAULT_PTTY        "/dev/console"
#define DEFAULT_CWD         "/home/zelda"

extern int main(int argc, char ** argv);
extern char **environ;

#define COM1_PORT 0x3f8

static uint8_t
inb(uint16_t portid)
{
    uint8_t ret;
    asm volatile("inb %1, %0;"
        :"=a"(ret)
        :"Nd"(portid));
    return ret;
}

static void
outb(uint16_t portid, uint8_t val)
{
    asm volatile("outb %%al, %%dx;"
        :
        :"a"(val), "Nd"(portid));
}


static int
is_transmit_empty(void)
{
    return inb(COM1_PORT + 5) & 0x20;
}

static void
write_serial(uint8_t a)
{
    while (is_transmit_empty() == 0);
    outb(COM1_PORT,a);
}
static void
print_serial(char * str)
{
     while(*str)
         write_serial(*str++);
}

static void
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
void _start(int argc, char ** argv)
{
    int32_t _start = (int)&_zelda_constructor_init_start;
    int32_t _end = (int)&_zelda_constructor_init_end;
    int32_t addr = 0;
    // Initialize the environ variable
    // with this initializator, we can then use getenv/setenv in stdlib
    // to manage the environment variables.
    environ = &argv[argc + 1];
    {
        // Initialize current working directory
        char * cwd = getenv(CWD_ENV_KEY);
        if (!cwd)
            cwd = DEFAULT_CWD;
        chdir(cwd);
    }
    // Initialize the standard IO
    {
        char * tty = getenv(PSEUDO_TERMINAL_KEY);
        if (!tty)
            tty = DEFAULT_PTTY;
        int fd_input = open(tty, O_RDONLY);
        int fd_out = open(tty, O_WRONLY);
        int fd_err = open(tty, O_WRONLY);
        if (fd_input != 0 || fd_out != 1 || fd_err != 2) {
            print_serial("Error in CRT0 stdio setup:\n    pid:0x");
            print_hexdecimal(getpid());
            print_serial("\n    ptty:");
            print_serial(tty);
            print_serial("\n    stdin:0x");
            print_hexdecimal((uint32_t)fd_input);
            print_serial("\n    stdout:0x");
            print_hexdecimal((uint32_t)fd_out);
            print_serial("\n    stderr:0x");
            print_hexdecimal((uint32_t)fd_err);
            print_serial("\n");
        }
    }
    // init_array as constructor, we run it here
    for(addr = _start; addr < _end; addr += 4)
        ((void (*)(void))*(int32_t *)addr)();
    // XXX: Introduce Constructor
    exit(main(argc, argv));
}
