/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PHYSICAL_MEMORY_H
#define _PHYSICAL_MEMORY_H
#include <lib/include/types.h>

/*
 * please refer to 
 * https://www.gnu.org/software/grub/manual/multiboot/html_node/multiboot_002eh.html
 */

struct multiboot_mmap {
    uint32_t size;
    uint32_t baseaddr_low;
    uint32_t baseaddr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_low;
    uint32_t mem_upper;

    uint32_t boot_device;
    uint32_t cmd_line;

    uint32_t mods_count;
    uint32_t mods_addr;

    uint32_t elf_num;
    uint32_t elf_size;
    uint32_t elf_addr;
    uint32_t elf_shindex;

    uint32_t mmap_length;
    uint32_t mmap_addr;
};

void probe_physical_memory(struct multiboot_info * boot_info);
__attribute__((always_inline)) inline uint32_t get_system_memory_boundary(void);
__attribute__((always_inline)) inline uint32_t get_system_memory_start(void);

extern uint8_t * _kernel_text_start;
extern uint8_t * _kernel_text_end;
extern uint8_t * _kernel_data_start;
extern uint8_t * _kernel_data_end;
extern uint8_t * _kernel_bss_start;
extern uint8_t * _kernel_bss_end;
extern uint8_t * _zelda_drive_start;
extern uint8_t * _zelda_drive_end;
extern uint8_t * _kernel_constructor_start;
extern uint8_t * _kernel_constructor_end;
#endif
