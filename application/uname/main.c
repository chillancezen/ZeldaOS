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
    printf("sysname(-s)  : %s\n", uts.sysname);
    printf("nodename(-n) : %s\n", uts.nodename);
    printf("release(-r)  : %s\n", uts.release);
    printf("version(-v)  : %s\n", uts.version);
    printf("machin(-m)   : %s\n", uts.machine);
    printf("domain(-d)   : %s\n", uts.domainname);
    return 0;
}
