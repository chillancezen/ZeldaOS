/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ZELDA_POSIX_H
#define _ZELDA_POSIX_H
#include <lib/include/types.h>

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




#endif
