/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdarg.h>
#include <syscall_inventory.h>
#include <zelda_posix.h>



int32_t
exit(int32_t exit_code)
{
    return do_system_call1(SYS_EXIT_IDX, exit_code);
}


int32_t
sleep(int32_t milisecond)
{
    return do_system_call1(SYS_SLEEP_IDX, milisecond);
}

int32_t
signal(int signal, void (*handler)(int32_t signal))
{
    return do_system_call2(SYS_SIGNAL_IDX, signal, (uint32_t)handler);
}

int32_t
kill(uint32_t task_id, int32_t signal)
{
    return do_system_call2(SYS_KILL_IDX, task_id, signal);
}

int32_t
open(const uint8_t * path, uint32_t flags, ...)
{
    int32_t mode = 0;
    va_list arg_ptr;
    va_start(arg_ptr, flags);
    mode = va_arg(arg_ptr, int32_t);
    return do_system_call3(SYS_OPEN_IDX, (uint32_t)path, flags, mode);
}


int32_t
close(uint32_t fd)
{
    return do_system_call1(SYS_CLOSE_IDX, fd);
}

int32_t
read(uint32_t fd, void * buffer, int32_t count)
{
    return do_system_call3(SYS_READ_IDX,fd, (uint32_t)buffer, count);
}

int32_t
write(uint32_t fd, void * buffer, int32_t count)
{
    return do_system_call3(SYS_WRITE_IDX, fd, (uint32_t)buffer, count);
}

int32_t
lseek(uint32_t fd, uint32_t offset, uint32_t whence)
{
    return do_system_call3(SYS_LSEEK_IDX, fd, offset, whence);
}

int32_t
stat(const uint8_t * path, struct stat * stat)
{
    return do_system_call2(SYS_STAT_IDX, (uint32_t)path, (uint32_t)stat);
}

int32_t
fstat(uint32_t fd, struct stat * stat)
{
    return do_system_call2(SYS_FSTAT_IDX, fd, (uint32_t)stat);
}

int32_t
getpid(void)
{
    return do_system_call0(SYS_GETPID_IDX);
}

void *
sbrk(uint32_t increment)
{
    return (void *)do_system_call1(SYS_SBRK_IDX, increment);
}


int32_t
isatty(uint32_t fd)
{
    return do_system_call1(SYS_ISATTY_IDX, fd);
}

// The maximum number of argumnets is 4
int32_t
ioctl(uint32_t fd, uint32_t request, ...)
{
    int32_t foo = 0;
    int32_t bar = 0;
    va_list arg_ptr;
    va_start(arg_ptr, request);
    foo = va_arg(arg_ptr, int32_t);
    bar = va_arg(arg_ptr, int32_t);
    return do_system_call4(SYS_IOCTL_IDX, fd, request, foo, bar);
}

uint8_t *
getcwd(uint8_t * buf, int32_t size)
{
    do_system_call2(SYS_GETCWD_IDX, (uint32_t)buf, size);
    return buf;
}

int32_t
chdir(const uint8_t * path)
{
    return do_system_call1(SYS_CHDIR_IDX, (uint32_t)path);
}

int32_t
execve(const uint8_t * path,
    uint8_t ** argv,
    uint8_t ** envp)
{
    return do_system_call3(SYS_EXECVE_IDX,
        (uint32_t)path,
        (uint32_t)argv,
        (uint32_t)envp);
}
int32_t
uname(struct utsname * uts)
{
    return do_system_call1(SYS_UNAME_IDX, (uint32_t)uts);
}
