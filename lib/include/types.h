/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _TYPES_H
#define _TYPES_H
#if 0
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif

#define NULL ((void *)0)

#define asm __asm__
#endif
