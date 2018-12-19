/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <kernel/include/printk.h>
#include <x86/include/gdt.h>
#include <x86/include/interrupt.h>
#include <x86/include/ioport.h>
#include <x86/include/pit.h>
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
#include <kernel/include/rtc.h>
#include <lib/include/heap_sort.h>
#include <kernel/include/timer.h>
#include <filesystem/include/devfs.h>
#include <device/include/pseudo_terminal.h>
#include <device/include/console.h>
#include <network/include/virtio_net.h>
#include <network/include/net_packet.h>
#include <network/include/ethernet.h>


static struct multiboot_info * boot_info;
static void
pre_init(void)
{
    uint32_t _start_addr = (uint32_t)&_kernel_constructor_start;
    uint32_t _end_addr = (uint32_t)&_kernel_constructor_end;
    uint32_t func_container = 0;
    serial_init();
    printk_init();
    for(func_container = _start_addr;
        func_container < _end_addr;
        func_container += 4) {
        ((void (*)(void))*(uint32_t *)func_container)();
    }
}

static void
init1(void)
{
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
    devfs_init();
}
static void
init5(void)
{
    net_packet_init();
    virtio_net_init();
}
static void
post_init(void)
{
   serial_post_init();
   ptty_post_init();
   console_init();
   timer_init();
   pci_post_init();
   task_init();
   ethernet_rx_post_init();
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
    init5();
    post_init();
    sti();
}
