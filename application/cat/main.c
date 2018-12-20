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
    printf("usage:/usr/bin/ls [-n] [/file/to/access]\n");
}

static void
print_file(const char * path, int flag_n)
{
#define ONESHOT_BUFFER_LEN  256
    int fd = 0;
    struct dirent dir;
    char buffer[ONESHOT_BUFFER_LEN];
    int32_t nr_read;
    int32_t nr_line = 0;
    int idx = 0;
    int32_t line_start = 0;
    int32_t line_end = 0;
    char line_buff[16];
    int32_t line_buffer_length = 0;
    if (getdents((uint8_t *)path, &dir, 1) != 1 ||
        dir.type != FILE_TYPE_REGULAR) {
        printf("error:%s not accessible or not a regular file\n",
            path);
        exit(-2);
    }
    fd = open((uint8_t *)path, O_RDONLY);

    if (fd < 0) {
        printf("error to open file:%s\n", path);
        exit(-3);
    }
    if (flag_n) {
        sprintf(line_buff, "%3d ", nr_line++);
        line_buffer_length = strlen(line_buff);
    }
    
    while ((nr_read = read(fd, buffer, ONESHOT_BUFFER_LEN)) > 0) {
        line_start = 0;
        line_end = 0;
        for (idx = 0; idx < nr_read; idx++) {
            if (buffer[idx] == '\n') {
                line_end = idx;
                // new line has been found
                if (line_buffer_length) {
                    write(1, line_buff, line_buffer_length);
                    line_buffer_length = 0;
                }
                write(1, &buffer[line_start], line_end - line_start + 1);
                if (flag_n) {
                    sprintf(line_buff, "%3d ", nr_line++);
                    line_buffer_length = strlen(line_buff);
                }
                line_start = idx + 1;
            }
        }
        // check one last time
        if (idx > line_start) {
            if (line_buffer_length) {
                write(1, line_buff, line_buffer_length);
                line_buffer_length = 0;
            }
            write(1, &buffer[line_start], idx - line_start);
        }
    }
    close(fd);
}
int
main(int argc, char * argv[])
{
    int flag_n = 0;
    int option = 0;

    while ((option = getopt(argc, argv,"n")) != -1) {
        switch (option)
        {
            case 'n':
                flag_n = 1;
                break;
            default:
                print_usage();
                exit(-1);
                break;
        }
    }
    if (optind == argc) {
        print_usage();
        exit(-1);
    }
    print_file(argv[optind], flag_n);
    return 0;
}
