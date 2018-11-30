/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_H
#define _ZELDA_H

#include <stdint.h>

void
clear_screen(void);

int32_t
pseudo_terminal_enable_master(void);

int32_t
pseudo_terminal_foreground(int32_t slave_task_id);

int32_t
pseudo_terminal_write_slave(void * buffer, int32_t size);

int
exec_command_line(const char * command_line,
    int (*command_hook)(const char * path,
        char ** argv,
        char ** envp));

#endif
