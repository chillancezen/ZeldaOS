/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>
#include <x86/include/timer.h>
#include <device/include/keyboard.h>
#include <memory/include/physical_memory.h>
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <device/include/serial.h>
#include <lib/include/string.h>
#include <memory/include/malloc.h>

static struct multiboot_info * boot_info;

static void
init1(void)
{
    serial_init();
    printk_init();
    gdt_init();
    idt_init();
}

static void
init2(void)
{
    probe_physical_mmeory(boot_info);
    paging_init();
    paging_fault_init();
    kernel_vma_init();
    malloc_init();
}
static void
init3(void)
{
    pit_init();
    keyboard_init();
}
static void
post_init(void)
{
   /*
    * switch stack to newly mapped stack top
    * NOTE that the invalid ESP will not cause page fault exception.
    * to work this around, we premap them before performing stack switching.
    * here we do walk through the STACK area. let the page fault handler
    * do it for us
    * UPDATE Aug 6, 2018: do not use page fault handler, it's slow and need
    * a lot of initial stack space, 2 MB is not far enough.
    */
   uint32_t stack_ptr = KERNEL_STACK_BOTTOM;
   LOG_INFO("Map kernel stack space:\n");
   disable_paging();
   for (; stack_ptr < KERNEL_STACK_TOP; stack_ptr += PAGE_SIZE) {
       //*(uint32_t *)stack_ptr = *(uint32_t *)stack_ptr;
       kernel_map_page(stack_ptr, get_page(), PAGE_PERMISSION_READ_WRITE);
   }
   enable_paging();

}
void kernel_main(struct multiboot_info * _boot_info, void * magicnum __used)
{
    boot_info = _boot_info;
    init1();
    init2();
    init3();
    sti();
    post_init();

    /*
     * perform stack switching with newly mapped stack area
     * prepare the return address of last frame in new stack
     */
    asm volatile("movl 4(%%ebp), %%eax;"
        "movl %0, %%ebx;"
        //"sub $0x4, %%ebx;" //actually, it's not necessary
        "movl %%ebx, %%esp;"
        "push %%eax;"
        "ret;"
        :
        :"i"(KERNEL_STACK_TOP)
        :"%eax", "%ebx");
    /*
     * do not put any code below this line
     * because the stack layout is incomplete
     *
     */
#if 0
    asm volatile("jmpl %0, $0x0;"
        :
        :"i"(TSS0_SELECTOR));
#endif
}
