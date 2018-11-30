/*
 * Copyright (c) 2018 Jie Zheng
 */

// The shell code for non-default consoles
#include <stdio.h>
#include <zelda.h>
#include <assert.h>

static void
shell_sigint_handler(int signal)
{
    printf("Hello world shell, exit\n");
    exit(0);
}
int main(int argc, char ** argv)
{
    struct utsname uts;
    char cmd_hint[128];
    char cwd[256];
    assert(!signal(SIGINT, shell_sigint_handler));
    pseudo_terminal_enable_master();
    memset(&uts, 0x0, sizeof(struct utsname));
    memset(cmd_hint, 0x0, sizeof(cmd_hint));
    assert(!uname(&uts));
    sprintf(cmd_hint, "[Link@%s.%s %s]# ", uts.nodename, uts.domainname,
        getcwd(cwd, sizeof(cwd)));
    while (1) {
        printf("%s\n", cmd_hint);
        sleep(1000);
    }
    return 0;
}
