/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <lib/include/string.h>

void
memset(void * dst, uint8_t target, int32_t size)
{
    uint8_t * ptr = (uint8_t *)dst;
    int idx =0 ;
    for(idx = 0; idx < size; idx++) {
        ptr[idx] = target;
    }
}
