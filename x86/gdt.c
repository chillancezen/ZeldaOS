/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/gdt.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>

#define _SEGMENT_BASE 0x0
#define _SEGMENT_LIMIT -1

#define _SEGMENT_TYPE_RW_DATA 0x2
#define _SEGMENT_TYPE_RX_CODE 0xa

__attribute__((aligned(8))) struct gdt_entry GDT[] = {
    //null segment
    {0},
    //kernel code segment
    {_SEGMENT_LIMIT, _SEGMENT_BASE, _SEGMENT_BASE, _SEGMENT_TYPE_RX_CODE, 1,
        DPL_0, 1, _SEGMENT_LIMIT, 0, 0, 1, 1, _SEGMENT_BASE},
    //kernel data segment
    {_SEGMENT_LIMIT, _SEGMENT_BASE, _SEGMENT_BASE, _SEGMENT_TYPE_RW_DATA, 1,
        DPL_0, 1, _SEGMENT_LIMIT, 0, 0, 1, 1, _SEGMENT_BASE},
    //user code segment
    {_SEGMENT_LIMIT, _SEGMENT_BASE, _SEGMENT_BASE, _SEGMENT_TYPE_RX_CODE, 1,
        DPL_3, 1, _SEGMENT_LIMIT, 0, 0, 1, 1, _SEGMENT_BASE},
    //user data segment
    {_SEGMENT_LIMIT, _SEGMENT_BASE, _SEGMENT_BASE, _SEGMENT_TYPE_RW_DATA, 1,
        DPL_3, 1, _SEGMENT_LIMIT, 0, 0, 1, 1, _SEGMENT_BASE}
};


void
gdt_init(void)
{
    struct gdt_pointer gdtr;
    gdtr.size = sizeof(GDT) -1;
    gdtr.offset = (uint32_t)(void*)GDT;
    asm volatile(
        "lgdt %0;"
        "movw $0x10, %%dx;"
        "movw %%dx, %%ds;"
        "movw %%dx, %%es;"
        "movw %%dx, %%fs;"
        "movw %%dx, %%gs;"
        "movw %%dx, %%ss;"
        "jmpl $0x08, $1f;"
        "1:"
        :
        :"m"(gdtr)
        :"%edx");
    LOG_INFO("load GDT with size:0x%x, base:0x%x\n", gdtr.size, gdtr.offset);
    dump_registers();
}
