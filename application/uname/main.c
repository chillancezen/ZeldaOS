/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdio.h>
#include <builtin.h>
#include <zelda.h>
#include <assert.h>
int
main(int argc, char * argv[])
{
    struct utsname uts;
    
    assert(!uname(&uts));
    printf("sysname  : %s\n", uts.sysname);
    printf("nodename : %s\n", uts.nodename);
    printf("release  : %s\n", uts.release);
    printf("version  : %s\n", uts.version);
    printf("machine  : %s\n", uts.machine);
    printf("domain   : %s\n", uts.domainname);
    return 0;
}
