/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _STRING_H
#define _STRING_H
#include <lib/include/types.h>

void memset(void * dst, uint8_t target, int32_t size);
void strcpy(uint8_t * dst, const uint8_t * src);
int start_with(const uint8_t * target, const uint8_t * sub);

#endif
