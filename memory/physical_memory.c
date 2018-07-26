/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/physical_memory.h>
#include <kernel/include/printk.h>

#define LOW_MEMORY_DELIMITER 0x100000


__used static uint32_t physical_memory_address = 0; //always count lower 1MB memory in
__used static uint32_t physical_memory_length = 0;

int32_t text_size_aligned = 0;
int32_t data_size_aligned = 0;
int32_t bss_size_aligned = 0;

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
    text_size_aligned = &_kernel_text_end - &_kernel_text_start;
    data_size_aligned = &_kernel_data_end - &_kernel_data_start;
    bss_size_aligned = &_kernel_bss_end - &_kernel_bss_start;
    
    text_size_aligned = (int32_t)page_align_addr(text_size_aligned);
    data_size_aligned = (int32_t)page_align_addr(data_size_aligned);
    bss_size_aligned = (int32_t)page_align_addr(bss_size_aligned);

    LOG_INFO("kernel text starts at 0x%x\n", &_kernel_text_start);
    LOG_INFO("kernel text ends at 0x%x (aligned size:%d)\n",
        &_kernel_text_end,
        text_size_aligned);
    LOG_INFO("kernel data starts at 0x%x\n", &_kernel_data_start);
    LOG_INFO("kernel data ends at 0x%x (aligned size:%d)\n",
        &_kernel_data_end,
        data_size_aligned);
    LOG_INFO("kernel bss starts at 0x%x\n", &_kernel_bss_start);
    LOG_INFO("kernel bss ends at 0x%x (aligned size:%d)\n",
        &_kernel_bss_end,
        bss_size_aligned);
}
