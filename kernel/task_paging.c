/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/task.h>
#include <kernel/include/elf.h>
#include <kernel/include/printk.h>
#include <memory/include/paging.h>
#include <lib/include/string.h>
#include <kernel/include/userspace_vma.h>
#include <x86/include/gdt.h>
int vma_in_task(struct task * task, struct vm_area * vma)
{
    struct list_elem * _list;
    struct vm_area * _vma;
    int _found = 0;
    LIST_FOREACH_START(&task->vma_list, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        if (_vma == vma){
            _found = 1;
            break;
        }
    }
    LIST_FOREACH_END();
    return _found;
}
void
dump_task_vm_areas(struct task * _task)
{
    struct list_elem * _list;
    struct vm_area * _vma;
    LOG_DEBUG("Dump task.vma_list\n");
    LIST_FOREACH_START(&_task->vma_list, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        LOG_DEBUG("   vma name:%s virt:0x%x length:0x%x permission:[%s%s]\n",
            _vma->name,
            (uint32_t)_vma->virt_addr,
            (uint32_t)_vma->length,
            _vma->write_permission == PAGE_PERMISSION_READ_WRITE ?
                "read-write" : "read-only",
            _vma->executable ? ",executable" : "");
    }
    LIST_FOREACH_END();
}

/*
 * Try to premap the VM area into task, if there is no enough memory(for
 *  both base page table for page directory and oridinary page table),
 *  return ERR_PARTIAL. the function do not roll back upon it.
 */
int
userspace_map_vm_area(struct task * task, struct vm_area * vma)
{
    int rc = 0;
    int found = 0;
    int partially_mapped = 0;
    struct list_elem * _list;
    struct vm_area * _vma;
    uint64_t addr = 0;
    uint32_t v_addr = 0;
    uint32_t p_addr = 0;

    LIST_FOREACH_START(&task->vma_list, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        if (_vma == vma) {
            found = 1;
            break;
        }
    }
    LIST_FOREACH_END();
    if (!found) {
        LOG_ERROR("The vma:0x%x is not in task:0x%x\n", vma, task);
        return -ERR_INVALID_ARG;
    }
    for (addr = vma->virt_addr;
        addr < (vma->virt_addr + vma->length); addr += PAGE_SIZE) {
        v_addr = (uint32_t)addr;
        // Prepare physical address
        if (!vma->exact) {
            p_addr = get_page();
            if (!p_addr) {
                LOG_DEBUG("can not allocate generic page for task:0x%x"
                    " vma:0x%x\n", task, vma);
                return partially_mapped ? -ERR_PARTIAL : -ERR_OUT_OF_MEMORY;
            }
        } else {
            p_addr = (uint32_t)(vma->phy_addr + addr - vma->virt_addr);
        }
        // Then try to map them.
        rc = userspace_map_page(task,
            v_addr,
            p_addr,
            vma->write_permission,
            vma->page_writethrough,
            vma->page_cachedisable);
        if (rc != OK) {
            if (!vma->exact)
                free_page(p_addr);
            return partially_mapped ? -ERR_PARTIAL : -ERR_OUT_OF_MEMORY;
        } else {
            partially_mapped = 1;
        }
    }
    return OK;
}

int
userspace_map_page(struct task * task,
    uint32_t virt_addr,
    uint32_t phy_addr,
    uint8_t write_permission,
    uint8_t page_writethrough,
    uint8_t page_cachedisable)
{
    uint32_t page_table = 0;
    uint32_t * page_table_ptr;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    struct pde32 * pde = NULL;
    struct pte32 * pte;
    ASSERT(task->page_directory);
    if (virt_addr < (uint32_t)USERSPACE_BOTTOM) {
        LOG_DEBUG("Not userspace virtual address:0x%x\n", virt_addr);
        return -ERR_INVALID_ARG;
    }
    pde = PDE32_PTR(&task->page_directory[pd_index]);
    if(!pde->present) {
        page_table = get_base_page();
        if(!page_table) {
            LOG_DEBUG("Failed to allocate page table for directory"
                " vaddr:0x%x\n",
                virt_addr);
            return -ERR_OUT_OF_MEMORY;
        }
        memset((void*)page_table, 0x0, PAGE_SIZE);
        task->page_directory[pd_index] = create_pde32(
            PAGE_PERMISSION_READ_WRITE,
            PAGE_PERMISSION_USER,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED,
            page_table);
        LOG_DEBUG("allocate page table for task:0x%x's page directory"
            " index:%x\n", task, pd_index);
        pde = PDE32_PTR(&task->page_directory[pd_index]);
    }
    ASSERT(pde->present);
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    page_table_ptr[pt_index] = create_pte32(
        write_permission,
        PAGE_PERMISSION_USER,
        page_writethrough,
        page_cachedisable,
        phy_addr);
    pte = PTE32_PTR(&page_table_ptr[pt_index]);
    ASSERT(pte->present);
    ASSERT((pte->pg_frame << 12) == (phy_addr & (~PAGE_MASK)));
    return OK;
}
/*
 * examine all the page table entries for their presence.
 * if all the page table entries are not present, reclaim the page table
 */
int
reclaim_page_table(struct task * task, uint32_t virt_addr)
{
    int idx = 0;
    int non_empty = 0;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    struct pde32 * pde = NULL;
    struct pte32 * pte = NULL;
    uint32_t * page_table_ptr = NULL;
    ASSERT(task->page_directory);
    pde = PDE32_PTR(&task->page_directory[pd_index]);
    if (!pde->present)
        return -ERR_NOT_PRESENT;
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    for (idx = 0; idx < 1024; idx++) {
         pte = PTE32_PTR(&page_table_ptr[idx]);
         if (pte->present) {
             non_empty = 1;
             break;
         }
    }
    if (non_empty) {
        return -ERR_BUSY;
    }
    free_base_page((uint32_t)page_table_ptr);
    task->page_directory[pd_index] = 0x0;
    LOG_DEBUG("reclaim task:0x%x's page directory entry:0x%x\n",
        task, pd_index);
    return OK;
}
/*
 * Evict a single page from task's directory
 * it only examine userspace pages and ignore kernel pages
 * if reclaim_page_table is set, the function will check whether the page table
 * is all non-present, reclaim it if so.
 * FIXME: Do not search vma here. it takes much time
 */
int
userspace_evict_page(struct task * task,
    uint32_t virt_addr,
    int _reclaim_page_table)
{
    uint32_t phy_page = 0;
    uint32_t pd_index = (virt_addr >> 22) & 0x3ff;
    uint32_t pt_index = (virt_addr >> 12) & 0x3ff;
    struct pde32 * pde = NULL;
    struct pte32 * pte = NULL;
    struct vm_area * _vma = NULL;
    uint32_t * page_table_ptr = NULL;
    ASSERT(task->page_directory);
    _vma = search_userspace_vma_by_addr(&task->vma_list, virt_addr);
    if (!_vma) {
        LOG_DEBUG("find no vm area for task:0x%x's virt_addr:0x%x\n",
            task, virt_addr);
        return -ERR_NOT_PRESENT;
    }
    pde = PDE32_PTR(&task->page_directory[pd_index]);
    if (!pde->present) {
        LOG_DEBUG("page directory entry:0x%x not present\n", pd_index);
        return -ERR_NOT_PRESENT;
    }
    page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
    pte = PTE32_PTR(&page_table_ptr[pt_index]);
    if (!pte->present) {
        LOG_DEBUG("page table entry:0x%x not present\n", pt_index);
        return -ERR_NOT_PRESENT;   
    }
    phy_page = (((uint32_t)pte->pg_frame) << 12);
    if (!_vma->exact) {
        free_page(phy_page);
    }
    page_table_ptr[pt_index] = 0x0;
    if (_reclaim_page_table) {
        reclaim_page_table(task, virt_addr);
    }
    return OK;
}
/*
 * Evict all pages in a vma, it automatically reclaim the unsed page 
 * tables and physical pages.
 * FIXME: Do not reclaim page directory each time to evict a page, instead,
 * accumulate the address space and call reclaim_page_table() once we are sure
 * to reclaim it.
 */
int
userspace_evict_vma(struct task * task, struct vm_area * vma)
{
    uint32_t next_page_directory = 0x0;
    uint32_t current_page_directory = 0x0;
    uint64_t addr = 0;
    if (!vma_in_task(task, vma))
        return -ERR_NOT_FOUND;
    if (vma->kernel_vma)
        return -ERR_INVALID_ARG;
    for (addr = vma->virt_addr;
        addr < (vma->virt_addr + vma->length); addr += PAGE_SIZE) {
        current_page_directory = (((uint32_t)addr) >> 22) & 0x3ff;
        next_page_directory = ((PAGE_SIZE + (uint32_t)addr) >> 22) & 0x3ff;
        userspace_evict_page(task, (uint32_t)addr,
            ((addr + PAGE_SIZE) >= (vma->virt_addr + vma->length)) ? 1 :
                current_page_directory != next_page_directory);
    }
    return OK;
}

/*
 * Load per-task page directory into PDBR(CR3), each time the per-task page
 * directory is about to be loaded, the kernel part of directory entries will
 * be copied to local directory.
 * TODO: need investigation on why Loading a different page directory into CR3
 * can cause a messy crash.
 * FIXME: currently we have a workaround: COPY pages directory entry to kernel
 * space directory, not in the opposite direction.
 * CAVEAT: for each task, we do the same operation, at very low risk.
 */
int
enable_task_paging(struct task * task)
{
    int idx = 0;
    uint32_t directory_index_top = USERSPACE_BOTTOM >> 12 >> 10;
    uint32_t * kernel_page_directory = (uint32_t *)get_kernel_page_directory();
    for(idx = directory_index_top; idx < 1024; idx++)
        kernel_page_directory[idx] = task->page_directory ?
            task->page_directory[idx] : 0x0;
    __asm__ volatile("movl %%eax, %%cr3;"
        :
        :"a"(kernel_page_directory));
    LOG_TRIVIA("enable page directory of task:0x%x\n", task);
    return OK;
}


int
userspace_remap_vm_area(struct task * task, struct vm_area * vma)
{
    uint32_t pd_index;
    uint32_t pt_index;
    struct pde32 * pde;
    struct pte32 * pte;
    uint32_t * page_table_ptr;
    uint64_t addr = 0;
    for (addr = vma->virt_addr;
        addr < (vma->virt_addr + vma->length); addr += PAGE_SIZE) {
        pd_index = (addr >> 22) & 0x3ff;
        pt_index = (addr >> 12) & 0x3ff;
        pde = PDE32_PTR(&task->page_directory[pd_index]);
        ASSERT(pde->present);
        page_table_ptr = (uint32_t *)(pde->pt_frame << 12);
        pte = PTE32_PTR(&page_table_ptr[pt_index]);
        ASSERT(pte->present);
        page_table_ptr[pt_index] = create_pte32(
            vma->write_permission,
            PAGE_PERMISSION_USER,
            vma->page_writethrough,
            vma->page_cachedisable,
            pte->pg_frame << 12);
    }
    return OK;   
}

uint32_t
handle_userspace_page_fault(struct task * task,
    struct x86_cpustate * cpu,
    uint32_t linear_addr)
{
    uint32_t result;
    uint32_t paddr = 0;
    struct vm_area * vma = NULL;
    ASSERT(task->page_directory);
    ASSERT(task->privilege_level == DPL_3);
    ASSERT(linear_addr >= ((uint32_t)USERSPACE_BOTTOM));
    vma = search_userspace_vma_by_addr(&task->vma_list, linear_addr);
    if (!vma) {
        return -ERR_NOT_FOUND;
    }
    if (vma->exact) {
        paddr = (uint32_t)(vma->phy_addr + linear_addr - vma->virt_addr);
    } else {
        paddr = get_page();
        if (!paddr) {
            LOG_DEBUG("can not allocate generic free page for task:0x%x's "
                "vma:0x%x\n", task, vma);
            return -ERR_OUT_OF_RESOURCE;
        }
    }
    result = userspace_map_page(task,
        linear_addr,
        paddr,
        vma->write_permission,
        vma->page_writethrough,
        vma->page_cachedisable);
    if (result != OK) {
        if (!vma->exact) {
            free_page(paddr);
        }
        return result;
    }
    LOG_TRIVIA("Finished mapping task:0x%s's vma:0x%x virt:0x%x to phy:0x%x",
        task, vma, linear_addr, paddr);
    return OK;
}
