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
enum log_level {
    LOG_TRIVIA = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_ASSERT
};
extern int __log_level;

#define LOG_TRIVIA(format, ...) {\
    if(__log_level <= LOG_TRIVIA) { \
        printk("[trivia] %s:%d ", __FILE__, __LINE__); \
        printk(format, ##__VA_ARGS__); \
    } \
}

#define LOG_DEBUG(format, ...) {\
    if (__log_level <= LOG_DEBUG) { \
        printk("[debug] %s:%d ", __FILE__, __LINE__); \
        printk(format, ##__VA_ARGS__); \
    } \
}

#define LOG_INFO(format, ...) { \
    if (__log_level <= LOG_INFO) { \
        printk("[info] %s:%d ", __FILE__, __LINE__); \
        printk(format, ##__VA_ARGS__); \
    } \
}

#define LOG_ERROR(format, ...) {\
    if (__log_level <= LOG_ERROR) { \
        printk("[error] %s:%d ", __FILE__, __LINE__); \
        printk(format, ##__VA_ARGS__); \
    } \
}

#define LOG_WARN(format, ...) {\
    if (__log_level <= LOG_WARN) { \
        printk("[warn] %s:%d ", __FILE__, __LINE__); \
        printk(format, ##__VA_ARGS__); \
    } \
}

#define ASSERT(cond) {\
    if (__log_level <= LOG_WARN) { \
        if (!(cond)){ \
            printk("[assert] %s:%d %s failed\n", __FILE__, __LINE__, #cond); \
            panic(); \
            asm volatile("1:cli;" \
                "hlt;" \
                "jmp 1b;"); \
        }\
    } \
}
#define VAR64(val)  (uint32_t)((val) >> 32), (uint32_t)(val)
#define __NOT_REACHED 0x0
#define __not_reach() ASSERT(__NOT_REACHED)
void hide_default_console(void);
void expose_default_console(void);
void reset_video_buffer(void);
void set_log_level(int);

int
vga_enqueue_byte(uint8_t target,
    void * opaque0 __used,
    void * opaque1 __used);
void
printk_flush(void);

#endif
