/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/physical_memory.h>
#include <kernel/include/printk.h>

#define LOW_MEMORY_DELIMITER 0x100000


__used static uint32_t physical_memory_address = 0; //always count lower 1MB memory in
__used static uint32_t physical_memory_length = 0;
__used static uint32_t system_memory_start = 0;

int32_t text_size_aligned = 0;
int32_t data_size_aligned = 0;
int32_t bss_size_aligned = 0;

__attribute__((always_inline)) inline
uint32_t get_system_memory_boundary(void)
{
        return physical_memory_length;
}

__attribute__((always_inline)) inline
uint32_t get_system_memory_start(void)
{
    uint32_t _bss_end = (uint32_t)&_kernel_bss_end;
    return (uint32_t)page_round_addr(_bss_end);
}
void probe_physical_memory(struct multiboot_info * boot_info)
{
    int idx = 0;
    struct multiboot_mmap * mmap =
        (struct multiboot_mmap*)boot_info->mmap_addr;
    struct multiboot_mmap * mmap_end =
        (struct multiboot_mmap*)(boot_info->mmap_addr + boot_info->mmap_length);
    LOG_INFO("Probing physical memory with multiboot:0x%x... ...\n",boot_info);
    for (idx = 0; mmap < mmap_end; mmap++, idx++) {
        LOG_INFO("   %d: type:%x baseaddr:0x%x length:0x%x %s\n",
            idx, mmap->type, mmap->baseaddr_low, mmap->length_low,
            LOW_MEMORY_DELIMITER == mmap->baseaddr_low? "(*system main memory)":
            mmap->baseaddr_high? "(*not addressable in ia32)": "");
        if (LOW_MEMORY_DELIMITER == mmap->baseaddr_low)
            physical_memory_length = mmap->baseaddr_low + mmap->length_low;
    }
    LOG_INFO("Set physical memory boundary:0x%x(%d MiB)\n",
        physical_memory_length,
        physical_memory_length/1024/1024);
    text_size_aligned =
        (int32_t)&_kernel_text_end - (int32_t)&_kernel_text_start;
    data_size_aligned =
        (int32_t)&_kernel_data_end - (int32_t)&_kernel_data_start;
    bss_size_aligned =
        (int32_t)&_kernel_bss_end - (int32_t)&_kernel_bss_start;
    
    text_size_aligned = (int32_t)page_round_addr(text_size_aligned);
    data_size_aligned = (int32_t)page_round_addr(data_size_aligned);
    bss_size_aligned = (int32_t)page_round_addr(bss_size_aligned);

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
    LOG_INFO("zelda drive starts at 0x%x\n", &_zelda_drive_start);
    LOG_INFO("Zelda drive ends at 0x%x(size:%d)\n", &_zelda_drive_end,
        ((uint32_t)&_zelda_drive_end) - ((uint32_t)&_zelda_drive_start));
    /*
     * check kernel image boundary, it should not exceed PAGE_SPACE_BOTTOM
     */
    ASSERT(((uint32_t)&_kernel_bss_end) <= PAGE_SPACE_BOTTOM);
}
