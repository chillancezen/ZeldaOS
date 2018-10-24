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


void
sleep(int32_t milisecond)
{
    do_system_call1(SYS_SLEEP_IDX, milisecond);
}
