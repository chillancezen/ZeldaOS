/*
 *Copyright (c) 2018 Jie Zheng
 */
#ifndef _PRINTK_H
#define _PRINTK_H
#include <lib/include/types.h>
#include <kernel/include/kernel.h>

void printk_init(void);
void printk_flush(void);
void printk(const char * format, ...);

#define LOG_INFO(format, ...) { \
    printk("[info] "); \
    printk(format, ##__VA_ARGS__); \
}

#define LOG_ERROR(format, ...) {\
    printk("[error] "); \
    printk(format, ##__VA_ARGS__); \
}

#define LOG_WARN(format, ...) {\
    printk("[warn] "); \
    printk(format, ##__VA_ARGS__); \
}

#define ASSERT(cond) {\
    if (!(cond)){ \
        printk("[assert] %s.%d %s failed\n", __FILE__, __LINE__, #cond); \
        panic(); \
        asm volatile("1:cli;" \
            "hlt;" \
            "jmp 1b;"); \
    }\
}
#define VAR64(val)  (uint32_t)((val) >> 32), (uint32_t)(val)
#endif
