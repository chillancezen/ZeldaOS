/*
 * Copyright (c) 2018 Jie Zheng
 */

/*
 *KERNEL_VMA_ARRAY_LENGTH is the length of kernel VMA array, this is
 *to be larger than 64
 */
#define KERNEL_VMA_ARRAY_LENGTH 1024


/*
 * Kernel heap bottom set to 0x8000000, i.e. 128MB
 * kernel heap top set to 0x1F000000 : 496MB
 * kernel stack bottom set to 0x1F000000 : 496MB
 * while kernel stack top is set to 0x20000000, i.e. 512 MB
 */
#define KERNEL_HEAP_BOTTOM 0x8000000
#define KERNEL_HEAP_TOP 0x1F000000
#define KERNEL_STACK_BOTTOM KERNEL_HEAP_TOP
#define KERNEL_STACK_TOP 0x20000000


/*
 *The MAX_FRAME_ON_DUMPSTACK indicates the maximum frames to dump
 *the calling stack.
 */
#define MAX_FRAME_ON_DUMPSTACK 64


#define DEFAULT_TASK_PRIVILEGE_STACK_SIZE (8*1024)


