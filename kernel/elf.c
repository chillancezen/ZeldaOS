/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/task.h>
#include <kernel/include/elf.h>
#include <lib/include/errorcode.h>
#include <kernel/include/printk.h>
#include <memory/include/malloc.h>
#include <x86/include/gdt.h>
#include <lib/include/string.h>
#include <memory/include/paging.h>

/*
 * This is to validate the content to check whther it's a legal elf32 statically 
 * formatted executable. 
 * dynamically linked executable is not intended to loaded. 
 */
int
validate_static_elf32_format(uint8_t * mem, int32_t length)
{
#define _(con) if (!(con)) goto out;
    int ret = OK;
    int idx = 0;
    struct elf32_elf_header * elf_hdr = (struct elf32_elf_header *)mem;
    struct elf32_program_header * program_hdr;
    _(length >= sizeof(struct elf32_elf_header));
    _((*(uint32_t *)elf_hdr->e_ident) == ELF32_IDENTITY_ELF);
    _(elf_hdr->e_ident[4] == ELF32_CLASS_ELF32);
    _(elf_hdr->e_ident[5] == ELF32_ENDIAN_LITTLE);
    _(elf_hdr->e_type == ELF32_TYPE_EXEC);
    _(elf_hdr->e_machine == ELF32_MACHINE_I386);
    _(elf_hdr->e_version == 0x1);
    _(elf_hdr->e_ehsize == sizeof(struct elf32_elf_header));
    _(elf_hdr->e_phentsize == sizeof(struct elf32_program_header));
    _(elf_hdr->e_shentsize == sizeof(struct elf32_section_header));
    /*
     * Process Program headers 
     */
    for (idx = 0; idx < elf_hdr->e_phnum; idx++) {
        program_hdr = (struct elf32_program_header *)(mem + elf_hdr->e_phoff
            + idx * sizeof(struct elf32_program_header));
        _((program_hdr->p_offset + program_hdr->p_filesz) < length);
        if (program_hdr->p_type == PROGRAM_TYPE_LOAD) {
            _(((uint64_t)(program_hdr->p_vaddr)) >= USERSPACE_BOTTOM);
            _(((uint64_t)(program_hdr->p_vaddr) + program_hdr->p_memsz)
                < USERSPACE_STACK_TOP);
        }
    }
    return ret;
    out:
        ret = -ERR_INVALID_ARG;
        return ret;
#undef _
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
 *Load ELF32 executable at PL3 as a tasks
 */
int
load_static_elf32(uint8_t * mem, uint8_t * command)
{
    int rc = 0;
    int idx = 0;
    int ret = OK;
    uint64_t heap_start = 0;
    int _text_and_data_counter = 0;
    uint8_t _text_and_data_vma_name[64];
    struct vm_area * _vma;
    struct list_elem * _list;
    struct task * _task = NULL;
    struct elf32_elf_header * elf_hdr = (struct elf32_elf_header *)mem;
    struct elf32_program_header * program_hdr;
    if (!(_task = malloc_task()))
        goto task_error;
    _task->privilege_level = DPL_3;
    /*
     * Note that PL0 stack space is still in kernel privileged space.
     *
     * */
    _task->privilege_level0_stack = malloc(DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
    if (!_task->privilege_level0_stack) {
        LOG_DEBUG("Can not allocate memory for PL0 stack\n");
        ret = -ERR_OUT_OF_MEMORY;
        goto task_error;
    }
    /*
     * 1. Prepare basic vm_areas in Task's vma_list
     * this also includes kernel's 1G space.
     */
    //kernelspace.vma VMA setup
    _vma = malloc(sizeof(struct vm_area));
    if (!_vma) {
        LOG_DEBUG("Can not allocate memory for VM area\n");
        ret = -ERR_OUT_OF_MEMORY;
        goto vma_error;
    }
    memset(_vma, 0x0, sizeof(struct vm_area));
    strcpy(_vma->name, (uint8_t *)KERNEL_VMA);
    _vma->pre_map = 0;
    _vma->exact = 0;
    _vma->write_permission = PAGE_PERMISSION_READ_ONLY;
    _vma->page_writethrough = PAGE_WRITEBACK;
    _vma->page_cachedisable = PAGE_CACHE_ENABLED;
    _vma->executable = 0;
    _vma->virt_addr = 0;
    _vma->phy_addr = 0;
    _vma->length = KERNELSPACE_TOP;
    list_append(&_task->vma_list, &_vma->list);
    //USER_VMA_TEXT_AND_DATA.0.....n
    for (idx = 0; idx < elf_hdr->e_phnum; idx++) {
        program_hdr = (struct elf32_program_header *)(mem + elf_hdr->e_phoff
            + idx * sizeof(struct elf32_program_header));
        if (program_hdr->p_type != PROGRAM_TYPE_LOAD)
            continue;
        if (!heap_start || heap_start < 
            (uint64_t)(program_hdr->p_vaddr + program_hdr->p_memsz))
            heap_start = program_hdr->p_vaddr + program_hdr->p_memsz;
        ASSERT(!(program_hdr->p_vaddr & PAGE_MASK));
        _vma = malloc(sizeof(struct vm_area));
        if (!_vma) {
            LOG_DEBUG("Can not allocate memory for VM area");
            ret = -ERR_OUT_OF_MEMORY;
            goto vma_error;  
        }
        memset(_vma, 0x0, sizeof(struct vm_area));
        sprintf((char *)_text_and_data_vma_name, "%s.%d", 
            USER_VMA_TEXT_AND_DATA, _text_and_data_counter++);
        strcpy(_vma->name, (uint8_t *)_text_and_data_vma_name);
        _vma->pre_map = 1;
        _vma->exact = 0;
        _vma->page_writethrough = PAGE_WRITEBACK;
        _vma->page_cachedisable = PAGE_CACHE_ENABLED;
        _vma->write_permission = program_hdr->p_flags & PROGRAM_WRITE ?
            PAGE_PERMISSION_READ_WRITE : PAGE_PERMISSION_READ_ONLY;
        _vma->executable = program_hdr->p_flags & PROGRAM_EXECUTE ? 1 : 0;
        _vma->virt_addr = program_hdr->p_vaddr;
        _vma->phy_addr = program_hdr->p_paddr;
        _vma->length = program_hdr->p_memsz;
        _vma->length = _vma->length & PAGE_MASK ?
            (_vma->length & (~PAGE_MASK)) + PAGE_SIZE : _vma->length;
        list_append(&_task->vma_list, &_vma->list);
    }
    // USER_VMA_HEAP vma setup
    heap_start = heap_start & PAGE_MASK ? 
        (heap_start & (~PAGE_MASK)) + PAGE_SIZE : heap_start;
    _vma = malloc(sizeof(struct vm_area));
    if (!_vma) {
        LOG_DEBUG("Can not allocate memory for VM area");
        ret = -ERR_OUT_OF_MEMORY;
        goto vma_error;
    }
    memset(_vma, 0x0, sizeof(struct vm_area));
    strcpy(_vma->name, (uint8_t *)USER_VMA_HEAP);
    _vma->pre_map = 0;
    _vma->exact = 0;
    _vma->page_writethrough = PAGE_WRITEBACK;
    _vma->page_cachedisable = PAGE_CACHE_ENABLED;
    _vma->write_permission = PAGE_PERMISSION_READ_WRITE;
    _vma->executable = 0;
    _vma->virt_addr = heap_start;
    _vma->phy_addr = 0;
    _vma->length = 0;
    list_append(&_task->vma_list, &_vma->list);
    // USER_VMA_STACK vma setup
    _vma = malloc(sizeof(struct vm_area));
    if (!_vma) {
        LOG_DEBUG("Can not allocate memory for VM area");
        ret = -ERR_OUT_OF_MEMORY;
        goto vma_error;
    }
    memset(_vma, 0x0, sizeof(struct vm_area));
    strcpy(_vma->name, (uint8_t *)USER_VMA_STACK);
    _vma->pre_map = 1;
    _vma->exact = 0;
    _vma->page_writethrough = PAGE_WRITEBACK;
    _vma->page_cachedisable = PAGE_CACHE_ENABLED;
    _vma->write_permission = PAGE_PERMISSION_READ_WRITE;
    _vma->executable = 0;
    _vma->virt_addr = USERSPACE_STACK_TOP -
        DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE;
    _vma->phy_addr = 0;
    _vma->length = DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE;
    list_append(&_task->vma_list, &_vma->list);
    dump_task_vm_areas(_task);
    /*
     * 2. Page directory setup and pre-map the text&data and stack vm area
     */
    _task->page_directory = (uint32_t *)get_base_page();
    if (!_task->page_directory) {
        LOG_DEBUG("Can not allocate memory for task.page_directory\n");
        ret = -ERR_OUT_OF_MEMORY;
        goto page_error;
    }
    memset(_task->page_directory, 0x0, PAGE_SIZE);
    // map the vma with premap set to 1
    LIST_FOREACH_START(&_task->vma_list, _list) {
        _vma = CONTAINER_OF(_list, struct vm_area, list);
        if (!_vma->pre_map)
            continue;
        rc = userspace_map_vm_area(_task, _vma);
        if (rc != OK) {
            LOG_ERROR("Can not pre-map task:0x%x's vma:0x%x(%s)\n",
                _task,
                _vma,
                _vma->name);
            ret = rc;
            goto page_error;
        }
    }
    LIST_FOREACH_END();


    printk("comparison:%d\n", ((uint32_t)USERSPACE_STACK_TOP) > (int32_t)0 );
    //printk("Heap begin:%x\n", *(uint32_t *)USERSPACE_BOTTOM);
    return ret;
    page_error:
    vma_error:
        while(!list_empty(&_task->vma_list)) {
            _list = list_pop(&_task->vma_list);
            ASSERT(_list);
            _vma = CONTAINER_OF(_list, struct vm_area, list);
            free(_vma);
        }
    task_error:
        if (_task) {
            if (_task->privilege_level0_stack)
                free(_task->privilege_level0_stack);
            if (_task->privilege_level3_stack)
                free(_task->privilege_level3_stack);
            free_task(_task);
        }
    return ret;
}
