/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <filesystem/include/devfs.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <kernel/include/task.h>

static void
write_hint(void)
{
    int idx = 0;
    int ret = 0;
    uint8_t hint[64];
    ASSERT(current);
    memset(hint, 0x0, sizeof(hint));
    ret = sprintf((char *)hint, "[%s:%d] ", current->name, current->task_id);
    for (idx = 0; idx < ret; idx++)
        vga_enqueue_byte(hint[idx], NULL, NULL);
    
}
static int32_t
console_dev_write(struct file * file, uint32_t offset, void * buffer, int size)
{
    int idx = 0;
    uint8_t * ptr = (uint8_t *)buffer;
    write_hint();
    for (idx = 0; idx < size; idx++) {
        vga_enqueue_byte(ptr[idx], NULL, NULL);
        if (ptr[idx] == '\n' && (idx + 1) < size && ptr[idx + 1])
            write_hint();
    }
    printk_flush();
    return size;
}

static struct file_operation console_dev_ops = {
    .isatty = NULL,
    .size = NULL,
    .stat = NULL,
    .read = NULL,
    .write = console_dev_write,
    .truncate = NULL,
    .ioctl = NULL
};

void
console_init(void)
{
    struct file_system * devfs = get_dev_filesystem();
    ASSERT(register_dev_node(devfs,
        (uint8_t *)"/console",
        0x0,
        &console_dev_ops,
        NULL));
}
