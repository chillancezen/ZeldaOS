/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/zeldafs.h>
#include <memory/include/physical_memory.h>
#include <kernel/include/printk.h>
#include <lib/include/list.h>

static uint32_t zelda_drive_start = (uint32_t)&_zelda_drive_start;
static uint32_t zelda_drive_end =(uint32_t)&_zelda_drive_end;

static struct list_elem head = {
    .prev = NULL,
    .next = NULL   
};

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
}
