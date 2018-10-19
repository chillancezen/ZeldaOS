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
#include <kernel/include/task.h>
#include <device/include/pci.h>
#include <device/include/ata.h>
#include <lib/include/generic_tree.h>
#include <filesystem/include/vfs.h>
#include <filesystem/include/zeldafs.h>
#include <filesystem/include/dummyfs.h>
#include <kernel/include/system_call.h>
#include <filesystem/include/memfs.h>

static struct multiboot_info * boot_info;
static void
pre_init(void)
{
    uint32_t _start_addr = (uint32_t)&_kernel_constructor_start;
    uint32_t _end_addr = (uint32_t)&_kernel_constructor_end;
    uint32_t func_container = 0;
    for(func_container = _start_addr;
        func_container < _end_addr;
        func_container += 4) {
        ((void (*)(void))*(uint32_t *)func_container)();
    }
}

static void
init1(void)
{
    serial_init();
    printk_init();
    gdt_init();
    idt_init();
    gp_init();
    system_call_init();
}

static void
init2(void)
{
    probe_physical_mmeory(boot_info);
    kernel_vma_init();
    paging_fault_init();
    paging_init();
    malloc_init();
}
static void
init3(void)
{
    pit_init();
    keyboard_init();
    pci_init();
    ata_init();
}
static void
init4(void)
{
    vfs_init();
    zeldafs_init();
    dummyfs_init();
    memfs_init();
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
    * UPDATE Oct 24, 2018: No need to switch to another stack area, since
    * in multitask context, every task has its own PL0 stack.
    */
#if 0
   uint32_t stack_ptr = KERNEL_STACK_BOTTOM;
   LOG_INFO("Map kernel stack space:\n");
   for (; stack_ptr < KERNEL_STACK_TOP; stack_ptr += PAGE_SIZE) {
        kernel_map_page(stack_ptr, get_page(),
            PAGE_PERMISSION_READ_WRITE,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED);
   }
#endif
   task_init();
   schedule_enable();
}
void kernel_main(struct multiboot_info * _boot_info, void * magicnum __used)
{
    boot_info = _boot_info;
    pre_init();
    init1();
    init2();
    init3();
    init4();
    post_init();
#if defined(INLINE_TEST)
    {
        int32_t result = 0x0;
        uint8_t buffer[256];
        struct file_entry entry;
        struct file  * file = NULL;
        file = do_vfs_open((uint8_t *)"/etc/dummy.cfg", 0, 0);
        do_vfs_open((uint8_t *)"/dummy/cute", 0, 0);
        entry.file = file;
        entry.offset = 0x0;
        memset(buffer, 0x0, sizeof(buffer));
        do_vfs_lseek(&entry, 10, SEEK_CUR);
        result = do_vfs_read(&entry, buffer, 10);
        printk("file: %d %d %s\n",result , entry.offset, buffer);
        memset(buffer, 0x0, sizeof(buffer));
        result = do_vfs_read(&entry, buffer, 10);
        printk("file: %d %d %s\n",result , entry.offset, buffer);
        memset(buffer, 0x0, sizeof(buffer));
        result = do_vfs_read(&entry, buffer, 10);
        printk("file: %d %d %s\n",result , entry.offset, buffer);

    }
    {
        struct mem_block_hdr hdr;
        int rc = 0;
        uint8_t buffer[256];
        memset(&hdr, 0x0, sizeof(struct mem_block_hdr));
        rc = mem_block_raw_write_random(&hdr, 4082, (uint8_t *)"hello world", 4096);
        rc = mem_block_raw_write_random(&hdr, 4083, (uint8_t *)"hello world22", 30);
        //rc = mem_block_raw_write_sequential(&hdr, (uint8_t *)"hello world", 501);
        printk("mem write rc:%d\n", rc);

        memset(buffer, 0x0, sizeof(buffer));
        rc =mem_block_raw_read(&hdr, 4082, buffer, 30);
        printk("mem read rc:%d %s\n", rc, buffer);
        rc = mem_block_raw_truncate(&hdr, 3);
        rc = mem_block_raw_truncate(&hdr, 0);
        rc = mem_block_raw_truncate(&hdr, 4085);
        rc = mem_block_raw_truncate(&hdr, 29);
        dump_mem_blocks(&hdr);
        printk("mem truncate rc:%d\n", rc); 
        memset(buffer, 0x0, sizeof(buffer));
        rc =mem_block_raw_read(&hdr, 0, buffer, 30);
        printk("mem read rc:%d %s\n", rc, buffer);
    }
    test_generic_tree();
#endif
    LOG_INFO("Finish initializing kernel...\n");
    sti();
    /*
     * perform stack switching with newly mapped stack area
     * prepare the return address of last frame in new stack
     * actually, this is not necessry, because we are going to run procedure
     * in task unit context.
     */
#if 0
    LOG_INFO("Switch stack to newly mapped space.\n");
    asm volatile("movl 4(%%ebp), %%eax;"
        "movl %0, %%ebx;"
        //"sub $0x4, %%ebx;" //actually, it's not necessary
        "movl %%ebx, %%esp;"
        "push %%eax;"
        "sti;"
        "ret;"
        :
        :"i"(KERNEL_STACK_TOP - 0x100)
        :"%eax", "%ebx");
    /*
     * do not put any code below this line
     * because the stack layout is incomplete
     *
     */
    asm volatile("jmpl %0, $0x0;"
        :
        :"i"(TSS0_SELECTOR));
#endif
}
