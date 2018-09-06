/*
 * Copyright (c) 2018 Jie Zheng
 */
int cute = 1;
int bar[1024*1024*10];

__attribute__((constructor)) void
foo(void)
{
    __asm__ volatile("movl $0x1, %%eax;"
        "int $0x80;"
        :
        :
        :"%eax");
}
extern void * _zelda_constructor_init_start;
extern void * _zelda_constructor_init_end;

int main(void)
{
    int _start = (int)&_zelda_constructor_init_start;
    int _end = (int)&_zelda_constructor_init_end;
    int addr = 0;
    for(addr = _start; addr < _end; addr += 4) {
        ((void (*)(void))*(int*)addr)();
    }
    return 0;
}
