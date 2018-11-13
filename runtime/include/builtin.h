/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _BUILTIN_H
#define _BUILTIN_H

#include <stdint.h>

int32_t
exit(int32_t exit_code);

int32_t
sleep(int32_t milisecond);

int32_t
signal(int signal, void (*handler)(int32_t signal));

int32_t
kill(uint32_t task_id, int32_t signal);

#endif
