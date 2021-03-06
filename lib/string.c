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
/*
 * XXX:make sure dst and src do not overlap.
 */
void
memcpy(void * dst, const void * src, int length)
{
    int idx = 0;
    uint8_t * _dst = (uint8_t *)dst;
    const uint8_t * _src = (const uint8_t *)src;
    for (idx = 0; idx < length; idx++)
        _dst[idx] = _src[idx];
}
int
strcmp(uint8_t * str1, uint8_t * str2)
{
    for(; *str1 && *str2 && *str1 == *str2; str1++, str2++);
    return *str1 - *str2;
}

__attribute__((deprecated)) void
strcpy(uint8_t * dst, const uint8_t * src)
{
    int idx = 0;
    for(; (dst[idx] = src[idx]); idx++);
}

void
strcpy_safe(uint8_t * dst, const uint8_t * src, int32_t max_size)
{
    int idx = 0;
    for (; idx < max_size && (dst[idx] = src[idx]); idx++);
}

uint32_t
strlen(const uint8_t * str)
{
    uint32_t iptr = 0;
    for(; str[iptr]; iptr++);
    return iptr;
}

int
strchr(const uint8_t * str, uint8_t target)
{
    int index = -1;
    const uint8_t * ptr = str;
    for(; *ptr; ptr++) {
        if (*ptr == target) {
            index = ptr - str;
            break;
        }
    }
    return index;
}
/*
 * This is to judge whether the target string starts with sub string
 * non-Zero indicates check failure.
 */
int
start_with(const uint8_t * target, const uint8_t * sub)
{
    int idx = 0;
    for(; sub[idx]; idx++)
        if(target[idx] != sub[idx])
            break;
    return sub[idx];
}
