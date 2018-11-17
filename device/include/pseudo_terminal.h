/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PSEUDO_TERMINAL
#define _PSEUDO_TERMINAL
#include <lib/include/types.h>
#include <filesystem/include/vfs.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>

#define PTTY_ROWS 25
#define PTTY_COLUMNS 80


struct pseudo_terminal_master {
    // Video buffer in memory, need to flush
    int32_t need_scroll;
    int32_t row_front;
    int32_t row_index;
    int32_t col_index;
    uint8_t buffer[PTTY_ROWS+1][PTTY_COLUMNS];
};

void
ptty_post_init(void);
#endif
