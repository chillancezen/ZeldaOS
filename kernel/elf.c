/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/task.h>
#include <kernel/include/elf.h>
#include <lib/include/errorcode.h>
#include <kernel/include/printk.h>

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
    }
    return ret;
    out:
        ret = -ERR_INVALID_ARG;
        return ret;
#undef _
}



int
load_static_elf32(uint8_t * mem,
    uint8_t * command,
    int DPL)
{
    struct task * _task = NULL;
    if (!(_task = malloc_task()))
        goto task_error;


    return OK;
    task_error:
        if (_task) {
            free_task(_task);
        }
    return -ERR_GENERIC;
}
