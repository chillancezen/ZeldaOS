/*
 * Copyright (c) 2018 Jie Zheng
 */

// This program is usually loaded by kernel code, and what it does is to
// initialize the rest of the userland applications

#include <stdio.h>
#include <stdlib.h>

#define INIT_CONFIG_FILE "/etc/userland.init"

int
main(int argc, char * argv[])
{
    int fd_config;
    printf("userland applications start\n");
    fd_config = open(INIT_CONFIG_FILE, O_RDONLY);
    if (fd_config < 0) {
        printf("can not open userland init config file\n");
        return -1;   
    }


    while (1) {
        sleep(1000);
        printf("Hello world\n");
    }
    return 0;
}
