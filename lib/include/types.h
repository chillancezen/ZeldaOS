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

#include <lib/include/errorcode.h>

#define NULL ((void *)0)
#define __used __attribute__((unused))
#define asm __asm__

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)

static inline uint32_t
page_round_addr(uint32_t addr)
{
    return (addr & PAGE_MASK) ?
        PAGE_SIZE + (uint32_t)(addr & (~PAGE_MASK)) :
        addr;
}



#define OFFSET_OF(structure, field) ((int32_t)(&(((structure *)0)->field)))
#define CONTAINER_OF(ptr, structure, field) \
    (structure *)(((uint32_t)(ptr)) - OFFSET_OF(structure, field))

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define HIGH_BYTE(word) ((uint8_t)(((word) >> 8) & 0xff))
#define LOW_BYTE(word) ((uint8_t)((word) & 0xff))
#define MAKE_WORD(high_byte, low_byte) \
    ((uint16_t)(((low_byte) & 0xff) | (((high_byte) << 8) & 0xff00)))

#define mfence() {__asm__ __volatile__("mfence;":::"memory");}
#define sfence() {__asm__ __volatile__("sfence;":::"memory");}
#define lfence() {__asm__ __volatile__("lfence;":::"memory");}
#endif
