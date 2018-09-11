/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/userspace_vma.h>
#include <lib/include/string.h>
#include <lib/include/errorcode.h>
#include <kernel/include/printk.h>

struct vm_area *
search_userspace_vma(struct list_elem * head, uint8_t * vma_name)
{
    struct vm_area * _vma = NULL;
    struct list_elem * _list;
    LIST_FOREACH_START(head, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        if(!strcmp(vma_name, _vma->name))
            break;
    }
    LIST_FOREACH_END();
    return _vma;
}

int
__extend_vm_area(struct vm_area * _vma, int direction, int length)
{
    switch(direction)
    {
        case VMA_EXTEND_UPWARD:
            _vma->virt_addr += length;
            _vma->phy_addr += length;
            break;
        case VMA_EXTEND_DOWNWARD:
            _vma->virt_addr -= length;
            _vma->phy_addr -= length;
            break;
        default:
            ASSERT(0);
    }
    _vma->length += length;
    return OK;
}

/*
 * Make sure target_vma in vmas_head list.
 * and the extended vma does not overlap with other areas
 */
int
extend_vm_area(struct list_elem * vmas_head,
    struct vm_area * target_vma,
    int direction,
    int length)
{
    int found = 0;

    uint64_t extended_start;
    uint64_t extended_end;
    struct list_elem * _list = NULL;
    struct vm_area * _vma = NULL;
    ASSERT(direction == VMA_EXTEND_UPWARD || direction == VMA_EXTEND_DOWNWARD);

    extended_start = direction == VMA_EXTEND_UPWARD ?
        target_vma->virt_addr :
        target_vma->virt_addr - length;
    extended_end = direction == VMA_EXTEND_UPWARD ?
        target_vma->virt_addr + target_vma->length + length:
        target_vma->virt_addr + target_vma->length; 
    LIST_FOREACH_START(vmas_head, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        if (_vma == target_vma) {
            found = 1;
            continue;
        }
        if (_vma->virt_addr < extended_start && 
            (_vma->virt_addr + _vma->length) > extended_start)
            return -ERR_INVALID_ARG;
        if (_vma->virt_addr < extended_end &&
            (_vma->virt_addr + _vma->length) > extended_end)
            return -ERR_INVALID_ARG;
    }
    LIST_FOREACH_END();

    if (!found) {
        return -ERR_INVALID_ARG;
    }
    return __extend_vm_area(target_vma, direction, length);
}
