/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PSEUDO_TERMINAL
#define _PSEUDO_TERMINAL
#include <lib/include/types.h>
#include <filesystem/include/vfs.h>
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>
#include <kernel/include/wait_queue.h>
#include <lib/include/ring.h>
#define PTTY_ROWS 25
#define PTTY_COLUMNS 80


struct pseudo_terminal_master {
    // the `master_task_id` is often to store the task of shell
    int32_t master_task_id;
    int32_t foreground_task_id;
    // Wait queue head, the task to wait input from pseudo terminal should
    // wait on it until it's woken by keyboad
    struct wait_queue_head wq_head;
    // Video buffer in memory, need to flush
    int32_t need_scroll;
    int32_t row_front;
    int32_t row_index;
    int32_t col_index;
    uint8_t buffer[PTTY_ROWS+1][PTTY_COLUMNS];
    // per-terminal input ring buffer
    __attribute__((aligned(4)))
        struct ring ring;
    // Do not inject any fields between `ring` and `buffer`
    uint8_t ring_buffer[PSEUDO_BUFFER_SIZE];
}__attribute__((packed));

extern struct pseudo_terminal_master * current_ptty;

void
ptty_post_init(void);
#endif
