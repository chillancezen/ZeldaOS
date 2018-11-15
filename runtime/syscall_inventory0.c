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
