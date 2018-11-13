/*
 * Copyright (c) 2018 Jie Zheng
 */

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

