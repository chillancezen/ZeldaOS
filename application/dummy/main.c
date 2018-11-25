/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


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
    ret = signal(SIGINT, interrupt_handler);
    while(1) {
        memset(buffer, 0x0, sizeof(buffer));
        gets(buffer);
        printf("Hello world:%s\n", buffer);
        sleep(100);
    }
    while(1)
        sleep(100);
    return 0;
}
