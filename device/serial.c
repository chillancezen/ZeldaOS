/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <x86/include/ioport.h>
#include <kernel/include/printk.h>
#include <filesystem/include/devfs.h>
#include <x86/include/interrupt.h>
#include <lib/include/ring.h>
#include <kernel/include/task.h>
#include <kernel/include/wait_queue.h>

#define COM1_INTERRUPT_VECTOR_NUMBER    36
#define COM1_PORT                       0x3f8
#define DEFAULT_SERIAL_RING_BUFFER_SIZE 64

static struct wait_queue_head wq_head;
static uint8_t local_buff_blob[
    sizeof(struct ring) + DEFAULT_SERIAL_RING_BUFFER_SIZE + 1];
static struct ring * serial_local_buff = (struct ring *)local_buff_blob;

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
    outb(COM1_PORT + 1, 0x01);// Disable all interrupts except `data available`
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
static int32_t
serial0_dev_read(struct file * file, uint32_t offset, void * buffer, int size)
{
    int nr_read = -ERR_GENERIC;
    struct wait_queue wait;
    ASSERT(current);
    initialize_wait_queue_entry(&wait, current);
    add_wait_queue_entry(&wq_head, &wait);

    while(ring_empty(serial_local_buff)) {
        transit_state(current, TASK_STATE_INTERRUPTIBLE);
        yield_cpu();
        if (signal_pending(current)) {
            nr_read = -ERR_INTERRUPTED;
            goto out;
        }
    }
    nr_read = read_ring(serial_local_buff, buffer, size);
    out:
        remove_wait_queue_entry(&wq_head, &wait);
        return nr_read; 
}
static struct file_operation serial0_dev_ops = {
    .size = serial0_dev_size,
    .stat = serial0_dev_stat,
    .read = serial0_dev_read,
    .write = serial0_dev_write,
    .truncate = NULL,
    .ioctl = NULL
};
static int32_t
serial_data_available(void)
{
    return inb(COM1_PORT + 5) & 1;
}

static uint8_t
serial_read(void)
{
    return inb(COM1_PORT);
}

static uint32_t
serial_port_interrupt_hadnler(struct x86_cpustate * cpu)
{
    uint32_t esp = (uint32_t)cpu;
    while(serial_data_available()) {
        ring_enqueue(serial_local_buff, serial_read());
    }
    wake_up(&wq_head);
    return esp;
}

void
serial_post_init(void)
{
    struct file_system * dev_fs = get_dev_filesystem();
    initialize_wait_queue_head(&wq_head);
    ASSERT(register_dev_node(dev_fs,
        (uint8_t *)"/serial0",
        0x0,
        &serial0_dev_ops,
        NULL));
    register_interrupt_handler(COM1_INTERRUPT_VECTOR_NUMBER,
        serial_port_interrupt_hadnler,
        "Serial Port Input Handler");
    serial_local_buff->ring_size = DEFAULT_SERIAL_RING_BUFFER_SIZE + 1;
    ring_reset(serial_local_buff);
}
