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
#ifndef NULL
    #define NULL ((void *)0)
#endif
int
exec_command_line(const char * command_line,
    int (*command_hook)(const char * path,
        char ** argv,
        char ** envp))
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
    if (command_hook)
        return command_hook(executable_path, argv, envp);
    return execve(executable_path, argv, envp);
}
