/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/multiboot_mmap.h>
#include <kernel/include/printk.h>

#define LOW_MEMORY_DELIMITER 0x100000


__used static uint32_t physical_memory_address = 0; //always count lower 1MB memory in
__used static uint32_t physical_memory_length = 0;

void probe_physical_mmeory(struct multiboot_info * boot_info)
{
    int idx = 0;
    struct multiboot_mmap * mmap =
        (struct multiboot_mmap*)boot_info->mmap_addr;
    struct multiboot_mmap * mmap_end =
        (struct multiboot_mmap*)(boot_info->mmap_addr + boot_info->mmap_length);
    LOG_INFO("Probing physical memory with multiboot:0x%x... ...\n",boot_info);
    for (idx = 0; mmap < mmap_end; mmap++, idx++) {
        LOG_INFO("%d: type:%x baseaddr:0x%x length:0x%x %s\n",
            idx, mmap->type, mmap->baseaddr_low, mmap->length_low,
            LOW_MEMORY_DELIMITER == mmap->baseaddr_low? "(*system main memory)":
            mmap->baseaddr_high? "(*not addressable in ia32)": "");
        if (LOW_MEMORY_DELIMITER == mmap->baseaddr_low)
            physical_memory_length = mmap->baseaddr_low + mmap->length_low;
    }
    LOG_INFO("Set physical memory boundary:0x%x(%d MiB)\n",
        physical_memory_length,
        physical_memory_length/1024/1024);
}
