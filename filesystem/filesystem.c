/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <filesystem/include/filesystem.h>
#include <kernel/include/printk.h>


#define _(_fs) #_fs
char * filesystem_name[] = {
    _(ZELDA_FS),
    _(DEV_FS),
    _(VSI_FS)
};
#undef _

char *
filesystem_type_to_name(int type)
{
    ASSERT(type >=0 && type < (sizeof(filesystem_name)/sizeof(char*)));
    return filesystem_name[type];
}
