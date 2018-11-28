/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_POSIX_H
#define _ZELDA_POSIX_H
#include <stdint.h>

/*
 * https://github.com/freebsd/freebsd/blob/master/sys/sys/stat.h#L160
 */
struct stat {
    uint16_t st_dev;
    uint16_t st_inode;
    uint32_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint16_t st_rdev;
    uint32_t st_size;
}__attribute__((packed));

// The File SEEK macro from: Linux/fs.h

#define SEEK_SET 0 /* seek relative to beginning of file */
#define SEEK_CUR 1 /* seek relative to current file position */
#define SEEK_END 2 /* seek relative to end of file */

// The File operations flags from: newlib/include/sys/_default_fcntl.h
// Must specify one in [O_RDONLY, O_WRONLY, O_RDWR], of course, by default is 
// 0: readonly
#define O_RDONLY 0x0
#define O_WRONLY 0x1
#define O_RDWR 0x2
#define O_ACCMODE 0x3 // O_RDONLY | O_WRONLY | O_RDWR

#define O_APPEND 0x0008
#define O_CREAT 0x0200
#define O_TRUNC 0x0400


/*
 * System call parameter delivery convention:
 * EAX: syscall number.
 * EBX: parameter 0
 * ECX: parameter 1
 * EDX: parameter 2
 * ESI: parameter 3
 * EDI: parameter 4
 * the maximum parameter in syscall is 5, and the parameter is about to be
 * pushed into stack (both in userspace and kernelspace) in the order of
 * right to left.
 */


enum SYSCALL_INDEX {
    SYS_OPEN_IDX = 0,
    SYS_CLOSE_IDX,
    SYS_READ_IDX,
    SYS_WRITE_IDX,
    SYS_LSEEK_IDX,
    SYS_STAT_IDX,
    SYS_FSTAT_IDX,
    SYS_EXIT_IDX,
    SYS_SLEEP_IDX,
    SYS_SIGNAL_IDX,
    SYS_KILL_IDX,
    SYS_GETPID_IDX,
    SYS_SBRK_IDX,
    SYS_ISATTY_IDX,
    SYS_IOCTL_IDX,
    SYS_GETCWD_IDX,
    SYS_CHDIR_IDX,
};

enum SIGNAL {
    SIG_INVALID = 0,
    SIGHUP = 1, // Hangup
    SIGINT = 2, // Interrupted CTRL^C
    SIGQUIT = 3, // Normal quit 
    SIGILL = 4,
    SIGTRAP = 5,
    SIGABRT = 6,
    SIGEMT = 7,
    SIGFPE = 8, // arighmetic exception.
    SIGKILL= 9, // Killed: kill -i pid
    SIGBUS = 10,
    SIGSEGV = 11, //Common Segmentation fault.
    SIGSYS = 12,
    SIGPIPE = 13,
    SIGALARM = 14, // Alarm cloc.
    SIGTERM = 15, // Terminated.
    SIGUSR1 = 16, // User defined signal 1
    SIGUSR2 = 17, // Another User defined signal
    SIGSTOP = 23, // task is to stop, can not catch it or ignored.
    SIGTSTP = 24, // Task is to stop, CTRL^Z
    SIGCONT = 25, // Task is to continue, it can not be catched.
    SIGTTIN = 26, // TTY input stopped.
    SIGTTOU = 27, // TTY output stopped.
    SIG_MAX,
};


// pseudo terminal ioctl request code
#define PTTY_IOCTL_CLEAR 0x1
#define PTTY_IOCTL_MASTER 0x2       // The master task id must be given
#define PTTY_IOCTL_FOREGROUND 0x3   // the slave task id must be given
#define PTTY_IOCTL_SLAVE_WRITE 0x4  // write to slave ring buffer

#endif
