/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <stdint.h>

extern void * _zelda_constructor_init_start;
extern void * _zelda_constructor_init_end;

extern int main(int argc, char ** argv);

int _start(int argc, char ** argv)
{
    int32_t _start = (int)&_zelda_constructor_init_start;
    int32_t _end = (int)&_zelda_constructor_init_end;
    int32_t addr = 0;
    // init_array as constructor, we run it here
    for(addr = _start; addr < _end; addr += 4)
        ((void (*)(void))*(int32_t *)addr)();
    // XXX: Introduce Constructor
    return main(argc, argv);
}
