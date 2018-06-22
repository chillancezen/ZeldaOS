/*
 *Copyright (c) 2018 Jie Zheng
 */
#ifndef _PRINTK_H
#define _PRINTK_H
#include <lib/include/types.h>
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
#define ASSERT(cond) {\
    if (!(cond)){ \
        printk("[assert] %s failed\n", #cond); \
    }\
}
#define VAR64(val)  (uint32_t)((val) >> 32), (uint32_t)(val)
void dump_registers(void);
#endif
