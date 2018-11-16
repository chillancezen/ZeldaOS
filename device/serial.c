/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <x86/include/ioport.h>
#include <kernel/include/printk.h>
#include <filesystem/include/devfs.h>

#define COM1_PORT 0x3f8

int
is_transmit_empty(void)
{
    return inb(COM1_PORT + 5) & 0x20;
}
 
 void
 write_serial(uint8_t a)
 {
    while (is_transmit_empty() == 0);   
    outb(COM1_PORT,a);
 }

void
serial_init(void)
{
    outb(COM1_PORT + 1, 0x00);// Disable all interrupts
    outb(COM1_PORT + 3, 0x80);// Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);// Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);//                  (hi byte)
    outb(COM1_PORT + 3, 0x03);// 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);// Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);// IRQs enabled, RTS/DSR set
    LOG_INFO("Initialize serial port output channel\n");
}

static int32_t
serial0_dev_size(struct file * file)
{
    return 0;
}
static int32_t
serial0_dev_stat(struct file * file, struct stat * stat)
{
    stat->st_size = 0;
    return OK;
}

static int32_t
serial0_dev_write(struct file * file, uint32_t offset, void * buffer, int size)
{
    int idx = 0;
    uint8_t * ptr = (uint8_t *)buffer;
    for (idx = 0; idx < size; idx++) {
        write_serial(ptr[idx]);
    }
    return size;
}
static struct file_operation serial0_dev_ops = {
    .size = serial0_dev_size,
    .stat = serial0_dev_stat,
    .read = NULL,
    .write = serial0_dev_write,
    .truncate = NULL,
    .ioctl = NULL
};

void
serial_post_init(void)
{
    struct file_system * dev_fs = get_dev_filesystem();
    ASSERT(register_dev_node(dev_fs,
        (uint8_t *)"/serial0",
        0x0,
        &serial0_dev_ops,
        NULL));
}
