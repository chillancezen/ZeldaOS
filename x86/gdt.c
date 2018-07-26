/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <x86/include/gdt.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <x86/include/tss.h>

#define _SEGMENT_BASE 0x0
#define _SEGMENT_LIMIT -1

#define _SEGMENT_TYPE_RW_DATA 0x2
#define _SEGMENT_TYPE_RX_CODE 0xa
#define _SEGMENT_TYPE_TSS 0x9

extern struct task_state_segment static_tss0;

#define TSS0_BASE ((uint32_t)&static_tss0)
#define TSS0_LIMIT (((uint32_t)sizeof(struct task_state_segment)) - 1)
//#define TSS0_LIMIT 0x67

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
        DPL_3, 1, _SEGMENT_LIMIT, 0, 0, 1, 1, _SEGMENT_BASE},
    //static tss0
    {0}
};


void
gdt_init(void)
{
    struct gdt_pointer gdtr;
    int tss_index = SELECTOR_INDEX(TSS0_SELECTOR);
    ASSERT(tss_index == 5);
    GDT[tss_index].limit_0_15 = TSS0_LIMIT & 0xffff;
    GDT[tss_index].base_0_15 = TSS0_BASE & 0xffff;
    GDT[tss_index].base_16_23 = (TSS0_BASE>>16) & 0xff;
    GDT[tss_index].segment_type = _SEGMENT_TYPE_TSS;
    GDT[tss_index].segmet_class = 0;
    GDT[tss_index].dpl = DPL_3;
    GDT[tss_index].present = 1;
    GDT[tss_index].limit_16_19 = (TSS0_LIMIT>>16) & 0xf;
    GDT[tss_index].avail = 0;
    GDT[tss_index].long_mode = 0;
    GDT[tss_index].operation_size = 0;
    GDT[tss_index].granularity = 0;
    GDT[tss_index].base_24_31 = (TSS0_BASE>>24) & 0xff;
    tss_init();
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
