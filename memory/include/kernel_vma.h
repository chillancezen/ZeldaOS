/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _KERNEL_VMA_H
#define _KERNEL_VMA_H
#include <lib/include/types.h>

#define KERNEL_VMA_NAME_SIZE 64
struct kernel_vma
{
    uint8_t name[KERNEL_VMA_NAME_SIZE];
    /*
     * a valid vma is set to 1
     */
    uint32_t present:1;
    /*
     * if the vma set exact to 1, the continuous phy_addr
     */
    uint32_t exact:1;
    uint32_t write_permission:1;
    uint32_t page_writethrough:1;
    uint32_t page_cachedisable:1;

    uint32_t premap:1;
    /*
     * both virt_addr and phy_addr must be PAGE_SIZE aligned
     */
    uint32_t virt_addr;
    uint32_t phy_addr;
    /*
     * length is times of PAGE_SIZE
     */
    uint32_t length;
};

struct kernel_vma * search_kernel_vma(uint32_t virt_addr);
int register_kernel_vma(struct kernel_vma * vma);
void kernel_vma_init(void);

void
dump_kernel_vma(void);

uint32_t
search_free_kernel_virtual_address(uint32_t length);

uint32_t
kernel_map_vma(
    uint8_t * vma_name,
    uint32_t exact,
    uint32_t premap,
    uint32_t phy_addr,
    uint32_t length,
    uint32_t write_permission,
    uint32_t page_writethrough,
    uint32_t page_cachedisable);

#endif
