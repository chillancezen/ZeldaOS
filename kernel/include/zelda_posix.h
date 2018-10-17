/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_POSIX_H
#define _ZELDA_POSIX_H


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
    SYS_EXIT_IDX,
};
#endif
