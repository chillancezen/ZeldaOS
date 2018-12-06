/*
 * Copyright (c) 2018 Jie Zheng
 */

// The shell code for non-default consoles
#include <stdio.h>
#include <zelda.h>
#include <assert.h>
#include <builtin.h>
#include <zelda.h>
#include <string.h>

int exit(int);

static void
shell_sigint_handler(int signal)
{
    printf("Shell exits\n");
    exit(0);
}

static char cmd_hint[128];
static void
update_cmd_hint(void)
{
    struct utsname uts;
    char cwd[256];
    memset(&uts, 0x0, sizeof(struct utsname));
    memset(cmd_hint, 0x0, sizeof(cmd_hint));
    assert(!uname(&uts));
    sprintf(cmd_hint, "[Link@%s.%s %s]# ", uts.nodename, uts.domainname,
        getcwd((uint8_t *)cwd, sizeof(cwd)));
}

static int
shell_clear(const char * path, char ** argv, char ** envp)
{
    clear_screen();
    return -ERR_PROCESSED;
}
static int
shell_exit(const char * path, char **argv, char ** envp)
{
    printf("shell is ready to exit...\n");
    exit(0);
    return -ERR_PROCESSED;
}
struct builtin_command_hander {
    char * command;
    int (*handler)(const char * path, char ** argv, char ** envp);
} shell_builtin_command[] = {
    {"clear", shell_clear},
    {"exit", shell_exit},
    {NULL, NULL},
};

extern char ** environ;

static int
process_shell_commands(const char * path,
    char ** argv,
    char ** envp)
{
    int idx = 0;
    for (idx = 0; ; idx++) {
        if (!shell_builtin_command[idx].command)
            break;;
        if (strcmp(shell_builtin_command[idx].command, path))
            continue;
        assert(shell_builtin_command[idx].handler);
        return shell_builtin_command[idx].handler(path, argv, envp);
    }
    if (!envp[0])
        return execve((const uint8_t *)path,
            (uint8_t **)argv,
            (uint8_t **)environ);
    return execve((const uint8_t *)path, (uint8_t **)argv, (uint8_t **)envp);
}
#define MAX_COMMAND_LINE_BUFFER_LENGTH 128
#define MAX_ONESHOT_BUFFER_LENGTH 32
#define HINT_CMD_LINE_FULL "[cmd line buffer full]"


int main(int argc, char ** argv)
{
    char command_line_buffer[MAX_COMMAND_LINE_BUFFER_LENGTH];
    char oneshot_buff[MAX_ONESHOT_BUFFER_LENGTH];
    int rc = 0;
    int iptr = 0;
    int iptr_inner = 0;
    int left = 0;
    int terminate = 0;
    int sub_task;
    assert(!signal(SIGINT, shell_sigint_handler));
    pseudo_terminal_enable_master();
    update_cmd_hint();
    {
        //Print welcome messages
        struct utsname uts;
        assert(!uname(&uts));
        printf("Welcome to %s Version %s\n", uts.sysname, uts.version);
        printf("Copyright (c) 2018 Jie Zheng [at] VMware\n\n");
    }
    while (1) {
        write(1, cmd_hint, strlen(cmd_hint));
        // prepare the command line
        memset(command_line_buffer, 0x0, sizeof(command_line_buffer));
        iptr = 0;
        terminate = 0;
        left = MAX_COMMAND_LINE_BUFFER_LENGTH;
        while (left > 0) {
            memset(oneshot_buff, 0x0, sizeof(oneshot_buff));
            rc = read(0, oneshot_buff, MAX_ONESHOT_BUFFER_LENGTH - 1);
            if (rc == -ERR_INTERRUPTED)
                continue;
            assert(rc > 0);
            for (iptr_inner = 0; iptr_inner < rc; iptr_inner++) {
                if (oneshot_buff[iptr_inner] == '\n') {
                    terminate = 1;
                    oneshot_buff[iptr_inner] = '\x0';
                    break;
                } else if ((iptr + 1) ==  MAX_COMMAND_LINE_BUFFER_LENGTH) {
                    write(1, HINT_CMD_LINE_FULL, strlen(HINT_CMD_LINE_FULL));
                    oneshot_buff[iptr_inner] = '\x0';
                    break;
                } else {
                    command_line_buffer[iptr++] = oneshot_buff[iptr_inner];
                    left -= 1;
                }
            }
            iptr_inner = strlen(oneshot_buff);
            if (iptr_inner)
                write(1, oneshot_buff, iptr_inner);
            if (terminate)
                break;
        }
        write(1, "\n", 1);
        // Got one command line, process the builtin commands first
        sub_task =
            exec_command_line(command_line_buffer, process_shell_commands);
        if (sub_task > 0) {
            while(wait0(sub_task));
        }
    }
    return 0;
}

__attribute__((constructor)) void
pre_init(void)
{
    memset(cmd_hint, 0x0, sizeof(cmd_hint));
}
