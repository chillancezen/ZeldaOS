/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
interrupt_handler(int signal)
{
    printf("Application Signal interrupted\n");
    sleep(100);
    printf("Application Signal interrupted ends\n");
    kill(-1, SIGKILL);
}
int main(int argc, char *argv[])
{
    char ch;
    int32_t ret = 0;
    char buffer[64];
    char * hint = "[link@hyrule.kingdom zelda] #";
    ret = signal(SIGINT, interrupt_handler);
    printf("pid:%d\n", getpid());
    while(1) {
        memset(buffer, 0x0, sizeof(buffer));
        ret = read(0, buffer, 64);
        write(1, buffer, ret);
        if (buffer[ret -1] == '\n') {
            write(1, hint, strlen(hint));
        }
    }
    while(1)
        sleep(100);
    return 0;
}
