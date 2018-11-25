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

int32_t
open(const uint8_t * path, uint32_t flags, ...);

int32_t
close(uint32_t fd);

int32_t
read(uint32_t fd, void * buffer, int32_t count);

int32_t
write(uint32_t fd, void * buffer, int32_t count);

int32_t
lseek(uint32_t fd, uint32_t offset, uint32_t whence);

int32_t
stat(const uint8_t * path, struct stat * stat);

int32_t
fstat(uint32_t fd, struct stat * stat);

int32_t
getpid(void);

void *
sbrk(uint32_t increment);

int32_t
isatty(uint32_t fd);

#endif
