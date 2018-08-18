/*
 * Copyright (c) 2018 Jie Zheng
 */

/*
 *KERNEL_VMA_ARRAY_LENGTH is the length of kernel VMA array, this is
 *to be larger than 64
 */
#define KERNEL_VMA_ARRAY_LENGTH 1024


/*
 * page space bottom set to 0x4000000, i.e. 64MB
 * Kernel heap bottom set to 0x8000000, i.e. 128MB
 * kernel heap top set to 0x1F000000 : 496MB
 * kernel stack bottom set to 0x1F000000 : 496MB
 * while kernel stack top is set to 0x20000000, i.e. 512 MB
 */

#define PAGE_SPACE_BOTTOM 0x4000000
#define PAGE_SPACE_TOP 0x8000000
#define KERNEL_HEAP_BOTTOM PAGE_SPACE_TOP
#define KERNEL_HEAP_TOP 0x1F000000
#define KERNEL_STACK_BOTTOM KERNEL_HEAP_TOP
#define KERNEL_STACK_TOP 0x20000000


/*
 *The MAX_FRAME_ON_DUMPSTACK indicates the maximum frames to dump
 *the calling stack.
 */
#define MAX_FRAME_ON_DUMPSTACK 64


#define DEFAULT_TASK_PRIVILEGED_STACK_SIZE (8*1024)
#define DEFAULT_TASK_NON_PRIVILEGED_STACK_SIZE (8*1024)


/*
 * enable/disable task preemption
 */
#define TASK_PREEMPTION 1

