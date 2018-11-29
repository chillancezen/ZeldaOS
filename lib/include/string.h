/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _STRING_H
#define _STRING_H
#include <lib/include/types.h>
// FIXME: make all string functions safe, and deprecate the unsafe functions
void memset(void * dst, uint8_t target, int32_t size);
void memcpy(void * dst, const void * src, int length);
int strcmp(uint8_t * str1, uint8_t * str2);

// deprecate unsafe version of strcpy
__attribute__((deprecated)) void
strcpy(uint8_t * dst, const uint8_t * src);

void
strcpy_safe(uint8_t * dst, const uint8_t * src, int32_t max_size);

uint32_t strlen(const uint8_t * str);
int start_with(const uint8_t * target, const uint8_t * sub);
int strchr(const uint8_t * str, uint8_t target);
// The function body is in: kernel/printk.c
int sprintf(char * dst, const char * format, ...);

#endif
