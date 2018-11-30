/*
 * Copyright (c) 2018 Jie Zheng
 */

// This program is usually loaded by kernel code, and what it does is to
// initialize the rest of the userland applications
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define INIT_CONFIG_FILE "/etc/userland.init"

static void *
load_config_file(const char * config_file)
{
    int fd_config;
    char * config_buffer = NULL;
    fd_config = open(INIT_CONFIG_FILE, O_RDONLY);
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
exec_command(const char * command_line)
{
#define QUOTE '"'
#define APOSTROPHE '\''
#define MAX_SEGMENTS 63
#define MAX_ARGS 31
#define MAX_ENVS 31
    int idx;
    int idx_tmp;
    char * ptr = NULL;
    char * ptr_another = NULL;
    char * start_ptr = NULL;
    char * end_ptr = NULL;
    char * start_stack[MAX_SEGMENTS + 1];
    char * end_stack[MAX_SEGMENTS + 1];
    char * argv[MAX_ARGS + 1];
    char * envp[MAX_ENVS + 1];
    int stack_ptr = 0;
    int executable_index = -1;
    int quote_count;
    int apostrophe_count;
    char local_command_buff[256];
    char * executable_path = NULL;
    memset(argv, 0x0, sizeof(argv));
    memset(envp, 0x0, sizeof(envp));
    memset(local_command_buff, 0x0, sizeof(local_command_buff));
    strcpy(local_command_buff, command_line);
    ptr = local_command_buff;
    while (*ptr) {
        for(; *ptr && *ptr == ' '; ptr++);
        if (!*ptr)
            break;
        start_ptr = ptr;
        end_ptr = start_ptr;
        
        quote_count = 0;
        apostrophe_count = 0;
        for (; *end_ptr; end_ptr++) {
            if (*end_ptr == ' ') {
                if (!(quote_count & 0x1) &&
                    !(apostrophe_count & 0x1))
                    break;   
            } else if (*end_ptr == QUOTE || *end_ptr == APOSTROPHE) {
                quote_count += *end_ptr == QUOTE ? 1 : 0;
                apostrophe_count += *end_ptr == APOSTROPHE ? 1 : 0;
                if (!(quote_count & 0x1) &&
                    !(apostrophe_count & 0x1))
                    break;
            }
        }
        if (!*ptr)
            break;
        start_stack[stack_ptr] = start_ptr;
        end_stack[stack_ptr] = end_ptr;
        stack_ptr++;
        ptr = end_ptr + 1;
    }

    for (idx = 0; idx < stack_ptr; idx++) {
        *end_stack[idx] = '\x0';
        ptr = start_stack[idx];
        ptr_another = start_stack[idx];
        for(; *ptr_another; ptr_another++) {
            if(*ptr_another == QUOTE || *ptr_another == APOSTROPHE)
                continue;
            *ptr++ = *ptr_another;
        }
        *ptr = '\x0';
    }
    for (idx = 0; idx < stack_ptr; idx++) {
        if (!strchr(start_stack[idx], '=')) {
            executable_index = idx;
            break;
        }
    }
    if (executable_index < 0) {
        return -1;
    }
    executable_path = start_stack[executable_index];

    for (idx = 0, idx_tmp = 0; idx < executable_index; idx++) {
        envp[idx_tmp++] = start_stack[idx];
    }
    for (idx = executable_index, idx_tmp = 0; idx < stack_ptr; idx++) {
        argv[idx_tmp++] = start_stack[idx];
    }
    return execve(executable_path, argv, envp);
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
        task_id = exec_command(ptr);
        printf("loadding;%s as task:%d\n", ptr, task_id);
        ptr = ptr_end + 1;
    }
    while (1) {
        sleep(1000);
    }
    return 0;
}
