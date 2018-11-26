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

#endif
