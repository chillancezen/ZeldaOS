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
 * parse commands and put environment variables and arguments
 * onto the PL3 stack.
 * http://ethv.net/weblog/004-program-loading-on-linux-3.html
 * note auxiliary vector is not supported for now.
 * the returned value is the new PL3 stack position
 */
uint32_t
resolve_commands(uint32_t pl3_stack_top, uint8_t * command)
{
#define QUOTE '\"'
#define APOSTROPHE '\''

    int length = 0;
    uint8_t * esp = (uint8_t *)pl3_stack_top;
    uint8_t * ptr = NULL;
    uint8_t * start_ptr = NULL;
    uint8_t * end_ptr = NULL;
    int quote_count;
    int apostrophe_count;
    length = strlen(command);
    esp -= length + 1;
    memcpy(esp, command, length + 1);
    ASSERT(esp[length] == '\x0');

    ptr = esp;
    esp = (uint8_t *)(((uint32_t)esp) & (~0x3));

    while(*ptr) {
        for(; *ptr && *ptr == ' '; ptr++);
        if(!*ptr)
            break; 
        start_ptr = ptr;
        end_ptr = start_ptr + 1;
        /*
         * Try to find word within a sematic,
         * i,e, an env variable, the application path name,
         * or an arumnent
         */
        quote_count = 0;
        apostrophe_count = 0;
        for (; *end_ptr; end_ptr++) {
            if (*end_ptr == ' ') {
                if (!(apostrophe_count & 0x1) &&
                    !(apostrophe_count & 0x1))
                    break;
            } else if (*end_ptr == QUOTE || *end_ptr == APOSTROPHE) {
                quote_count += *end_ptr == QUOTE ? 1 : 0;
                apostrophe_count += *end_ptr == APOSTROPHE ? 1 : 0;
                if (!(apostrophe_count & 0x1) &&
                    !(apostrophe_count & 0x1))
                    break;
            }
        }
        //if (*ptr)
    }
    printk("%x %x\n", esp, ptr);
    return pl3_stack_top;
}
/*
 *Load ELF32 executable at PL3 as a task
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
    struct x86_cpustate * _cpu = NULL;
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
    _task->privilege_level0_stack_top =
        ((uint32_t)_task->privilege_level0_stack) +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE -
        0x100;
    LOG_DEBUG("task:0x%x's PL0 stack:0x%x<---->0x%x\n",
        _task,
        _task->privilege_level0_stack,
        ((uint32_t)_task->privilege_level0_stack) +
            DEFAULT_TASK_PRIVILEGED_STACK_SIZE);
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
    _vma->kernel_vma = 1;
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
        _vma->kernel_vma = 0;
        _vma->pre_map = 1;
        _vma->exact = 0;
        _vma->page_writethrough = PAGE_WRITEBACK;
        _vma->page_cachedisable = PAGE_CACHE_ENABLED;
        _vma->write_permission = PAGE_PERMISSION_READ_WRITE;
        /*
         * Temporarily make any text&data vma writable.
         * after copying the program segmet, re-map it as needed.
         */
        //_vma->write_permission = program_hdr->p_flags & PROGRAM_WRITE ?
        //    PAGE_PERMISSION_READ_WRITE : PAGE_PERMISSION_READ_ONLY;
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
    _vma->kernel_vma = 0;
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
    _vma->kernel_vma = 0;
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
    /*
     * 3. switch to task paging directory base and load the elf's program
     * segment into task memory
     */
    enable_task_paging(_task);
    /*
     * 4. copy program segments into VMA.
     */
    for (idx = 0; idx < elf_hdr->e_phnum; idx++) {
        program_hdr = (struct elf32_program_header *)(mem + elf_hdr->e_phoff
            + idx * sizeof(struct elf32_program_header));
        if (program_hdr->p_type != PROGRAM_TYPE_LOAD)
            continue;
        memcpy((void *)program_hdr->p_vaddr,
            (void *)(((uint32_t)mem) + program_hdr->p_offset),
            (int)program_hdr->p_filesz);
        _vma = search_userspace_vma_by_addr(&_task->vma_list,
            (uint32_t)program_hdr->p_vaddr);
        ASSERT(_vma);
        _vma->write_permission = program_hdr->p_flags & PROGRAM_WRITE ?
            PAGE_PERMISSION_READ_WRITE : PAGE_PERMISSION_READ_ONLY;
        userspace_remap_vm_area(_task, _vma);
    }
    /*
     * 5. Prepare initial PL0 stack.
     */
    _task->entry = elf_hdr->e_entry;
    _vma = search_userspace_vma(&_task->vma_list, (uint8_t *)USER_VMA_STACK);
    ASSERT(_vma);
    _cpu = (struct x86_cpustate *)((((uint32_t)_task->privilege_level0_stack) +
        DEFAULT_TASK_PRIVILEGED_STACK_SIZE -
        sizeof(struct x86_cpustate) -
        0x40) & (~0xf));
    memset(_cpu, 0x0, sizeof(struct x86_cpustate));
    _cpu->ss = USER_DATA_SELECTOR;
    _cpu->esp = resolve_commands(
        (uint32_t)(_vma->virt_addr + _vma->length - 0x10),
        command);
    ASSERT(!(_cpu->esp & 0x3));
    _cpu->eflags = EFLAGS_ONE | EFLAGS_INTERRUPT | EFLAGS_PL3_IOPL;
    _cpu->cs = USER_CODE_SELECTOR;
    _cpu->eip = elf_hdr->e_entry;
    _cpu->errorcode = 0;
    _cpu->vector = 0;
    _cpu->ds = USER_DATA_SELECTOR;
    _cpu->es = USER_DATA_SELECTOR;
    _cpu->fs = USER_DATA_SELECTOR;
    _cpu->gs = USER_DATA_SELECTOR;
    _cpu->eax = 0;
    _cpu->ecx = 0;
    _cpu->edx = 0;
    _cpu->ebx = 0;
    _cpu->ebp = 0;
    _cpu->esi = 0;
    _cpu->edi = 0;
    _task->cpu = _cpu;
    /*
     * 6. Prepare to be scheduled.
     */
    enable_kernel_paging();
    task_put(_task);
    return ret;
    page_error:
        LIST_FOREACH_START(&_task->vma_list, _list) {
            _vma = CONTAINER_OF(_list, struct vm_area, list);
            if (!_vma->kernel_vma)
                userspace_evict_vma(_task, _vma);
        }
        LIST_FOREACH_END();
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
            free_task(_task);
        }
    return ret;
}
