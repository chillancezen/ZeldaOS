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

/*
 *Load ELF32 executable at PL3 as a tasks
 */
int
load_static_elf32(uint8_t * mem, uint8_t * command)
{
    int ret = OK;
    struct vm_area * _vma;
    struct task * _task = NULL;
    //struct elf32_elf_header * elf_hdr = (struct elf32_elf_header *)mem;
    //struct elf32_program_header * program_hdr;
    if (!(_task = malloc_task()))
        goto task_error;
    _task->privilege_level = DPL_3;
    /*
     * Note that PL0 stack space is still in kernel privileged space.
     *
     * */
    _task->privilege_level0_stack = malloc(DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
    if (!_task->privilege_level0_stack) {
        ret = -ERR_OUT_OF_MEMORY;
        goto task_error;
    }
    /*
     * 1. Prepare basic vm_areas in Task's vma_list
     * this also includes kernel's 1G space.
     */
    //kernelspace.vma VMA setup
    _vma = malloc(sizeof(struct vm_area));
    if(!_vma) {
        ret = -ERR_OUT_OF_MEMORY;
        goto vma_error;   
    }
    memset(_vma, 0x0, sizeof(struct vm_area));
    strcpy(_vma->name, (uint8_t *)KERNEL_VMA);
    _vma->exact = 0;
    _vma->write_permission = PAGE_PERMISSION_READ_ONLY;
    _vma->page_writethrough = PAGE_WRITEBACK;
    _vma->page_cachedisable = PAGE_CACHE_ENABLED;
    _vma->executable = 1;
    _vma->virt_addr = 0;
    _vma->phy_addr = 0;
    _vma->length = KERNELSPACE_TOP;
    list_append(&_task->vma_list, &_vma->list);
    //USER_VMA_TEXT_AND_DATA.0.....n
    return ret;
    vma_error:

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
