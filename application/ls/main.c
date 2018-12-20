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
#define MAX_DIRENT      (1024)
int
getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

static void
print_usage(void)
{
    printf("usage:/usr/bin/ls [-a] [-l] [/path/to/access]\n");
}

static void
print_directory(struct dirent * dirp,
    int nr_dir,
    int flag_a,
    int flag_l)
{
    int idx = 0;
    for (idx = 0; idx < nr_dir; idx++) {
        if (!flag_a && dirp[idx].type == FILE_TYPE_MARK)
            continue;
        if (flag_l) {
            printf("%-20s  %-6d   %s\n",
                dirp[idx].type == FILE_TYPE_MARK ? "FILE_TYPE_MARK" :
                    dirp[idx].type == FILE_TYPE_REGULAR ? "FILE_TYPE_REGULAR" :
                    "FILE_TYPE_DIR",
                dirp[idx].size,
                dirp[idx].type == FILE_TYPE_MARK ? "." :
                    (char *)dirp[idx].name);
        } else {
            printf("%10s   ", dirp[idx].type == FILE_TYPE_MARK ?
                "." : (char *)dirp[idx].name);
        }
    }
    printf("\n");
}
int
main(int argc, char * argv[])
{
    int flag_a = 0;
    int flag_l = 0;
    int option = 0;
    char cwd[256];
    struct dirent * dirp = malloc(MAX_DIRENT * sizeof(struct dirent));
    int32_t nr_dir = 0;
    int32_t idx = 0;

    while ((option = getopt(argc, argv,"al")) != -1) {
        switch (option)
        {
            case 'a':
                flag_a = 1;
                break;
            case 'l':
                flag_l = 1;
                break;
            default:
                print_usage();
                exit(-1);
                break;
        }
    }
    for (idx = optind; idx < argc; idx++) {
        nr_dir = getdents((uint8_t *)argv[idx], dirp, MAX_DIRENT);
        if (flag_l)
            printf("%s:\n", argv[idx]);
        if (nr_dir > 0)
            print_directory(dirp, nr_dir, flag_a, flag_l);
    }
    if (optind == argc) {
        getcwd((uint8_t *)cwd, sizeof(cwd));
        nr_dir = getdents((uint8_t *)cwd, dirp, MAX_DIRENT);
        if (nr_dir > 0)
            print_directory(dirp, nr_dir, flag_a, flag_l);
    }
    return 0;
}
