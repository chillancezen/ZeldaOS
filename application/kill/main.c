/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <stdio.h>
#include <builtin.h>
#include <zelda.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

int
getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;


static void
print_usage(void)
{
        printf("usage:/usr/bin/kill [-s signum] pid\n");
}

int
main(int argc, char * argv[])
{
    int32_t sig_number = -1;
    int32_t task_id = -1;
    int32_t result = 0;
    int option = 0;
    while ((option = getopt(argc, argv,"s:")) != -1) {
        switch (option)
        {
            case 's':
                sig_number = atoi(optarg);
                break;
            default:
                print_usage();
                exit(-1);
                break;
        }
    }
    if (sig_number < 0 || optind >= argc) {
        print_usage();
        exit(-2);
    }
    task_id = atoi(argv[optind]);
    result = kill(task_id, sig_number);
    if (result) {
        printf("error:%d in sending signal:%d to task:%d\n",
            result, sig_number, task_id);
    }
    return 0;
}
