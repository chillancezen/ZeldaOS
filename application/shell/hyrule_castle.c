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
#include <stdlib.h>
//int exit(int);
static int outgoing_task_id = -1;
static void
shell_sigint_handler(int signal)
{
    if (outgoing_task_id > 0) {
        // When shell receive SIGINT signals, it immediately send the SIGINT to
        // the outgoing sub task. by default the ZELDA runtime specify a handler
        // to gracefully terminate the task itself
        kill(outgoing_task_id, SIGINT);
    }
}
static char last_wd[256];
static char search_path[256];
static char cmd_hint[128];
static char global_cwd[256];
static char * tty;
static char * path_to_search;
static void
update_cmd_hint(void)
{
    struct utsname uts;
    memset(&uts, 0x0, sizeof(struct utsname));
    memset(cmd_hint, 0x0, sizeof(cmd_hint));
    assert(!uname(&uts));
    sprintf(cmd_hint, "[Link@%s.%s %s]# ", uts.nodename, uts.domainname,
        getcwd((uint8_t *)global_cwd, sizeof(global_cwd)));
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
static int
shell_cd(const char * path, char ** argv, char ** envp)
{
    char * target_wd = argv[1] ? argv[1] : last_wd;
    uint8_t cwd[256];
    getcwd(cwd, sizeof(cwd));
    int32_t error;
    if ((error = chdir((uint8_t *)target_wd))) {
        printf("unable to change current working directory, error:%d\n", error);
    } else {
        strcpy(last_wd, (char *)cwd);
        update_cmd_hint();
    }
    return -ERR_PROCESSED;
}
static int
shell_pwd(const char * path, char ** argv, char ** envp)
{
    uint8_t cwd[256];
    printf("%s\n", getcwd(cwd, sizeof(cwd)));
    return -ERR_PROCESSED;
}
struct builtin_command_hander {
    char * command;
    int (*handler)(const char * path, char ** argv, char ** envp);
} shell_builtin_command[] = {
    {"clear", shell_clear},
    {"exit", shell_exit},
    {"cd", shell_cd},
    {"pwd", shell_pwd},
    {NULL, NULL},
};

extern char ** environ;
static inline int
__isalpha(int ch)
{
    return (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z');
}

static inline int
__isdigit(int ch)
{
    return ch >= '0' && ch <= '9';
}
static int
process_shell_commands(const char * _path,
    char ** argv,
    char ** envp)
{
    int idx = 0;
    char path[256];
    memset(path, 0x0, sizeof(path));
    for (; *_path && *_path == ' '; _path++);
    strcpy(path, _path);
    if (__isalpha(_path[0]) || __isdigit(_path[0])) {
        // Given the paths to search, we compose a potential absolute path
        // here before we proceed
        char tmp_buffer[256];
        char split_buffer[256];
        char delimiter[] = ":";
        char * path_token = NULL;
        memset(split_buffer, 0x0, sizeof(split_buffer));
        strcpy(split_buffer, search_path);
        struct dirent dir;
        path_token = strtok(split_buffer, delimiter);
        do {
            if (!path_token)
                break;
            sprintf(tmp_buffer, "%s/%s", path_token, _path);
            if (getdents((uint8_t *)tmp_buffer, &dir, 1) == 1 &&
                dir.type == FILE_TYPE_REGULAR) {
                memset(path, 0x0, sizeof(path));
                strcpy(path, tmp_buffer);
                break;
            }
        } while ((path_token = strtok(NULL, delimiter)));
    }
    for (idx = 0; ; idx++) {
        if (!shell_builtin_command[idx].command)
            break;;
        if (strcmp(shell_builtin_command[idx].command, path))
            continue;
        assert(shell_builtin_command[idx].handler);
        return shell_builtin_command[idx].handler(path, argv, envp);
    }
    if (!envp[0]) {
        // XXX:here we only care environment variable: PATH, tty, cwd
        // where PATH and tty are inherited from shell, cwd varies.
        char local_environ[4][256];
        char * local_envp[4];
        int local_env_iptr = 0;
        if (tty) {
            sprintf(local_environ[local_env_iptr], "tty=%s", tty);
            local_envp[local_env_iptr] = local_environ[local_env_iptr];
            local_env_iptr++;
        }
        sprintf(local_environ[local_env_iptr], "PATH=%s", search_path);
        local_envp[local_env_iptr] = local_environ[local_env_iptr];
        local_env_iptr++;

        sprintf(local_environ[local_env_iptr], "cwd=%s", global_cwd);
        local_envp[local_env_iptr] = local_environ[local_env_iptr];
        local_env_iptr++;

        local_envp[local_env_iptr] = NULL;
        return execve((const uint8_t *)path,
            (uint8_t **)argv,
            (uint8_t **)local_envp);
    }
    return execve((const uint8_t *)path, (uint8_t **)argv, (uint8_t **)envp);
}
#define MAX_COMMAND_LINE_BUFFER_LENGTH 128
#define MAX_ONESHOT_BUFFER_LENGTH 32
#define HINT_CMD_LINE_FULL "[cmd line buffer full]"
#define DEFAUT_SEARCH_PATH  "/usr/bin:/usr/sbin"

int main(int argc, char ** argv)
{
    int is_serial0 = 0;
    char command_line_buffer[MAX_COMMAND_LINE_BUFFER_LENGTH];
    char oneshot_buff[MAX_ONESHOT_BUFFER_LENGTH];
    int rc = 0;
    int iptr = 0;
    int iptr_inner = 0;
    int left = 0;
    int terminate = 0;
    int sub_task;
    tty = getenv("tty");
    path_to_search = getenv("PATH");
    memset(last_wd, 0x0, sizeof(last_wd));
    memset(search_path, 0x0, sizeof(search_path));
    getcwd((uint8_t *)last_wd, sizeof(last_wd));
    strcpy(search_path, path_to_search ? path_to_search : DEFAUT_SEARCH_PATH);
    is_serial0 = tty && !strcmp(tty, "/dev/serial0");
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
            if (iptr_inner && !is_serial0)
                write(1, oneshot_buff, iptr_inner);
            if (terminate)
                break;
        }
        if (!is_serial0)
            write(1, "\n", 1);
        // Got one command line, process the builtin commands first
        // strip '\r' one more time, in /dev/serial0, an extra '\r' is put
        // into the buffer
        {
            for (iptr = 0; iptr < MAX_COMMAND_LINE_BUFFER_LENGTH; iptr++) {
                if (command_line_buffer[iptr] == '\r') {
                    command_line_buffer[iptr] = '\x0';
                    break;
                }
            }
        }
        sub_task =
            exec_command_line(command_line_buffer, process_shell_commands);
        if (sub_task > 0) {
            outgoing_task_id = sub_task;
            while(wait0(sub_task));
            outgoing_task_id = -1;
        }
    }
    return 0;
}

__attribute__((constructor)) void
pre_init(void)
{
    memset(cmd_hint, 0x0, sizeof(cmd_hint));
}
