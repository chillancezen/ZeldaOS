/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>


__attribute__((always_inline)) static inline int32_t
do_system_call0(int32_t call_num)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num));
    return ret;
}

__attribute__((always_inline)) static inline int32_t
do_system_call1(int32_t call_num, uint32_t arg0)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num), "b"(arg0));
    return ret;
}

__attribute__((always_inline)) static inline int32_t
do_system_call2(int32_t call_num, uint32_t arg0, uint32_t arg1)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num), "b"(arg0), "c"(arg1));
    return ret;
}

__attribute__((always_inline)) static inline int32_t
do_system_call3(int32_t call_num, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num), "b"(arg0), "c"(arg1), "d"(arg2));
    return ret;
}

__attribute__((always_inline)) static inline int32_t
do_system_call4(int32_t call_num,
    uint32_t arg0,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3));
    return ret;
}


__attribute__((always_inline)) static inline int32_t
do_system_call5(int32_t call_num,
    uint32_t arg0,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3,
    uint32_t arg4)
{
    int32_t ret = 0x0;
    __asm__ volatile("int $0x87;"
        :"=a"(ret)
        :"a"(call_num), "b"(arg0), "c"(arg1), "d"(arg2), "S"(arg3), "D"(arg4));
    return ret;
}
