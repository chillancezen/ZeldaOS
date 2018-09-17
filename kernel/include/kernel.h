/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _KERNEL_H
#define _KERNEL_H
#include <lib/include/types.h>
void gp_init(void);

void dump_registers(void);
void panic(void);

#endif
