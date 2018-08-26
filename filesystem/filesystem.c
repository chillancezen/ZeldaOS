/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <filesystem/include/filesystem.h>


#define _(_fs) #_fs
char * filesystem_name[] = {
    _(ZELDA_FS),
    _(DEV_FS),
    _(VSI_FS)
};
#undef _
