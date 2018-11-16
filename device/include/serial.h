/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _SERIAL_H
#define _SERIAL_H
#include <lib/include/types.h>
void serial_init(void);
void write_serial(uint8_t a);
void serial_post_init(void);

#endif
