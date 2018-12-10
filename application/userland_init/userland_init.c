/*
 * Copyright (c) 2018 Jie Zheng
 */

// This program is usually loaded by kernel code, and what it does is to
// initialize the rest of the userland applications
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <builtin.h>
#include <zelda.h>
#define INIT_CONFIG_FILE "/etc/userland.init"

static void *
load_config_file(const char * config_file)
{
    int fd_config;
    char * config_buffer = NULL;
    fd_config = open((uint8_t *)INIT_CONFIG_FILE, O_RDONLY);
    if (fd_config < 0) {
        printf("can not open userland init config file\n");
        return NULL;
    }
    {
        struct stat _stat;
        assert(!fstat(fd_config, &_stat));
        if (_stat.st_size <= 0)
            return NULL;
        config_buffer = malloc(_stat.st_size + 0x10);
        assert(config_buffer);
        memset(config_buffer, 0x0, _stat.st_size + 0x10);
        {
            int nleft = _stat.st_size;
            int iptr = 0;
            int rc = 0;
            while (nleft > 0) {
                rc = read(fd_config, config_buffer + iptr, nleft);
                if (rc <= 0)
                    break;
                nleft -= rc;
                iptr += rc;
            }
        }
    }
    close(fd_config);
    return config_buffer;
}


int
main(int argc, char * argv[])
{
    
    char * ptr = NULL;
    char * ptr_end = NULL;
    void * config_buff = NULL;
    int task_id;
    printf("userland applications start\n");
    config_buff = load_config_file(INIT_CONFIG_FILE);
    ptr = config_buff;
    while (*ptr) {
        for (ptr_end = ptr + 1; *ptr_end && *ptr_end != '\n'; ptr_end++);
        *ptr_end = '\0';
        task_id = exec_command_line(ptr, NULL);
        printf("loading:%s as task:%d\n", ptr, task_id);
        //while(wait0(task_id));
        ptr = ptr_end + 1;
    }
    while (1) {
        sleep(1000);
    }
    return 0;
}
