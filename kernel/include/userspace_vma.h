/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _USERSPACE_VMA_H
#define _USERSPACE_VMA_H
#include <lib/include/types.h>
#include <lib/include/list.h>

#define VM_AREA_NAME_SIZE 64
struct vm_area {
    uint8_t name[VM_AREA_NAME_SIZE];
    struct list_elem list;

    uint32_t exact:1;
    uint32_t write_permission:1;
    uint32_t page_writethrough:1;
    uint32_t page_cachedisable:1;
    uint32_t readable:1;
    uint32_t writable:1;
    uint32_t executable:1;

    uint64_t virt_addr;
    uint64_t phy_addr;

    uint32_t length;
};

#define USER_VMA_TEXT "userspace.vma.text"
#define USER_VMA_DATA "userspace.vma.data"
#define USER_VMA_HEAP "userspace.vma.heap"
#define USER_VMA_STACK "userspace.vma.stack"

#define VMA_EXTEND_UPWARD 0x1
#define VMA_EXTEND_DOWNWARD 0x2

struct vm_area *
search_userspace_vma(struct list_elem * head, uint8_t * vma_name);

int __extend_vm_area(struct vm_area * _vma, int direction, int length);
#endif
