/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdarg.h>
#include <syscall_inventory.h>
#include <zelda_posix.h>


void
clear_screen(void)
{
    ioctl(1, PTTY_IOCTL_CLEAR);
}

int32_t
pseudo_terminal_enable_master(void)
{
    uint32_t pid = getpid();
    return ioctl(0, PTTY_IOCTL_MASTER, pid);
}


int32_t
pseudo_terminal_foreground(int32_t slave_task_id)
{
    return ioctl(0, PTTY_IOCTL_FOREGROUND, slave_task_id);
}

int32_t
pseudo_terminal_write_slave(void * buffer, int32_t size)
{
    return ioctl(1, PTTY_IOCTL_SLAVE_WRITE, buffer, size);
}

void
__assert_fail (const char *assertion,
    const char *file,
    unsigned int line,
    const char *function)
{
    printf("crtn failure: %s: %u: %s%sAssertion `%s' failed!\n",
            file, line, function ?: "", function ? ": " : "",
            assertion);
    exit(-1);
}
