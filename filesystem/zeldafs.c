/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/zeldafs.h>
#include <memory/include/physical_memory.h>
#include <kernel/include/printk.h>
#include <lib/include/list.h>
#include <lib/include/string.h>
#include <kernel/include/elf.h>

static uint32_t zelda_drive_start = (uint32_t)&_zelda_drive_start;
static uint32_t zelda_drive_end =(uint32_t)&_zelda_drive_end;

static struct list_elem head = {
    .prev = NULL,
    .next = NULL   
};

struct zelda_file *
search_zelda_file(char * name)
{
    struct zelda_file * _file;
    struct list_elem * _list;
    LIST_FOREACH_START(&head, _list) {
        _file = CONTAINER_OF(_list, struct zelda_file, list);
        if (!strcmp(_file->path, (uint8_t *)name))
            return _file;
    }
    LIST_FOREACH_END();
    return NULL;
}

void
dump_zedla_drives(void)
{
    struct zelda_file * _file;
    struct list_elem * _list;
    LOG_INFO("Dump Zelda Drive:\n");
    LIST_FOREACH_START(&head, _list) {
        _file = CONTAINER_OF(_list, struct zelda_file, list);
        LOG_INFO("   File name:%s size:%d\n", _file->path, _file->length);
    }
    LIST_FOREACH_END();
}


void
enumerate_files_in_zelda_drive(void)
{
    struct zelda_file * _file = NULL;
    uint32_t iptr = zelda_drive_start;
    for(; iptr < zelda_drive_end;) {
        _file = (struct zelda_file *)iptr; 
        _file->list.next = NULL;
        _file->list.prev = NULL;
        list_append(&head, &_file->list);
        iptr += sizeof(struct zelda_file) + _file->length;
    }
}


void
zeldafs_init(void)
{
    enumerate_files_in_zelda_drive();
    dump_zedla_drives();
    {
        int rc = 0, rc1;
        struct zelda_file * _file = search_zelda_file("/usr/bin/dummy");
        printk("found zelda file:%x\n", _file);
        rc = validate_static_elf32_format(_file->content, _file->length);
        rc1 = load_static_elf32(_file->content, (uint8_t *)"");
        printk("%d %d\n", rc, rc1);
    }
}
