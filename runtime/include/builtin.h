/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _BUILTIN_H
#define _BUILTIN_H

#include <stdint.h>

#if 0
int32_t
exit(int32_t exit_code);
#endif

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

int32_t
ioctl(uint32_t fd, uint32_t request, ...);

uint8_t *
getcwd(uint8_t * buf, int32_t size);

int32_t
chdir(const uint8_t * path);

int32_t
execve(const uint8_t * path,
    uint8_t ** argv,
    uint8_t ** envp);

int32_t
uname(struct utsname * uts);

int32_t
wait0(int32_t target_task_id);

int32_t
getdents(uint8_t * path,
    struct dirent * dirp,
    int32_t count);

#endif
