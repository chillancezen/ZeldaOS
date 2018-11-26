/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <filesystem/include/devfs.h>
#include <device/include/pseudo_terminal.h>
#include <device/include/keyboard.h>
#include <device/include/keyboard_scancode.h>
#include <kernel/include/task.h>
#include <x86/include/ioport.h>

#define VIDEO_MEMORY_BASE 0xb8000
#define CONSOLE_CURSOR_PORT0 0x3d4
#define CONSOLE_CURSOR_PORT1 0x3d5

static uint16_t (*video_ptr)[PTTY_COLUMNS] = (void *)VIDEO_MEMORY_BASE;
static struct pseudo_terminal_master ptties[MAX_TERMINALS];
struct pseudo_terminal_master * current_ptty = NULL;

int32_t
ptty_init(struct pseudo_terminal_master * ptm)
{
    memset(ptm, 0x0, sizeof(struct pseudo_terminal_master));
    ptm->ring.ring_size = PSEUDO_BUFFER_SIZE;
    ring_reset(&ptm->ring);
    ASSERT(ring_empty(&ptm->ring));
    initialize_wait_queue_head(&ptm->wq_head);

    ptm->slave_ring.ring_size = PSEUDO_BUFFER_SIZE;
    ring_reset(&ptm->slave_ring);
    ASSERT(ring_empty(&ptm->slave_ring));
    initialize_wait_queue_head(&ptm->slave_wq_head);

    return OK;
}

int32_t
ptty_enqueue_byte(struct pseudo_terminal_master * ptm, uint8_t value)
{
    int idx;
    int32_t last_row_index = ptm->row_index;
    if (value != '\n')
        ptm->buffer[ptm->row_index][ptm->col_index] = value;
    ptm->col_index++;
    if (ptm->col_index == PTTY_COLUMNS || value == '\n') {
        ptm->col_index = 0;
        ptm->row_index = (ptm->row_index + 1) % (PTTY_ROWS + 1);
        if (((ptm->row_index + 1) % (PTTY_ROWS + 1)) == ptm->row_front) {
            for (idx = 0; idx < PTTY_COLUMNS; idx++)
                ptm->buffer[ptm->row_front][idx] = 0;
            ptm->row_front = (ptm->row_front + 1) % (PTTY_ROWS + 1);
        }
    }
    ptm->need_scroll |= last_row_index != ptm->row_index; 
    return OK;
}

static void
update_cusor(int x, int y);

int32_t
ptty_flush_terminal(struct pseudo_terminal_master * ptm)
{
    int idx_row = 0;
    int idx_col = 0;
    int32_t video_row_idx = 0;
    if (!ptm->need_scroll) {
        video_row_idx = (ptm->row_index + PTTY_ROWS + 1 -
            ptm->row_front) % (PTTY_ROWS + 1);
        ASSERT(video_row_idx < PTTY_ROWS);
        for (idx_col = 0; idx_col < PTTY_COLUMNS; idx_col++)
            video_ptr[video_row_idx][idx_col] =
                (video_ptr[video_row_idx][idx_col] & 0xff00) |
                ptm->buffer[ptm->row_index][idx_col];
        update_cusor(ptm->col_index, video_row_idx + 1);
    } else {
        for (idx_row = ptm->row_front;
            idx_row != ((ptm->row_index + 1) % (PTTY_ROWS + 1));
            idx_row = (idx_row + 1) % (PTTY_ROWS + 1), video_row_idx++) {
            for (idx_col = 0; idx_col < PTTY_COLUMNS; idx_col++) {
                video_ptr[video_row_idx][idx_col] =
                    (video_ptr[video_row_idx][idx_col] & 0xff00) |
                    ptm->buffer[idx_row][idx_col];
            }
        }
        update_cusor(ptm->col_index, video_row_idx);
    }
    ptm->need_scroll = 0;
    return OK;
}

static int32_t
__ptty_master_read(struct file * file, uint32_t offset, void * buffer, int size)
{
    int32_t result = -ERR_GENERIC;
    struct wait_queue wait;
    struct pseudo_terminal_master * ptm  = file->priv;
    // must be in task context, if not, return immediately
    ASSERT(current);
    ASSERT(ptm);
    ASSERT(current->task_id == ptm->master_task_id);

    initialize_wait_queue_entry(&wait, current);
    add_wait_queue_entry(&ptm->wq_head, &wait);
    // wait until the master ring buffer is not empty
    while (ring_empty(&ptm->ring)) {
        transit_state(current, TASK_STATE_INTERRUPTIBLE);
        yield_cpu();
        if (signal_pending(current)) {
            result = -ERR_INTERRUPTED;
            goto out;
        }
    }
    result = read_ring(&ptm->ring, buffer, size);
    out:
        remove_wait_queue_entry(&ptm->wq_head, &wait);
        return result;
}

static int32_t
__ptty_slave_read(struct file * file, uint32_t offset, void * buffer, int size)
{
    int32_t result = -ERR_GENERIC;
    struct wait_queue wait;
    struct pseudo_terminal_master * ptm = file->priv;

    initialize_wait_queue_entry(&wait, current);
    add_wait_queue_entry(&ptm->slave_wq_head, &wait);

    while (ptm->foreground_task_id != current->task_id ||
        ring_empty(&ptm->slave_ring)) {
        transit_state(current, TASK_STATE_INTERRUPTIBLE);
        yield_cpu();
        if (signal_pending(current)) {
            result = -ERR_INTERRUPTED;
            goto out;
        }
    }
    result = read_ring(&ptm->slave_ring, buffer, size);
    out:
        remove_wait_queue_entry(&ptm->slave_wq_head, &wait);
        return result;
}

static int32_t
ptty_dev_read(struct file * file, uint32_t offset, void * buffer, int size)
{
    struct pseudo_terminal_master * ptm = file->priv;
    if (!current)
        return -ERR_INVALID_ARG;
    if (ptm->master_task_id == current->task_id) {
        return __ptty_master_read(file, offset, buffer, size);
    }
    return __ptty_slave_read(file, offset, buffer, size);
}

static int32_t
ptty_dev_write(struct file * file, uint32_t offset, void * buffer, int size)
{
    int idx = 0;
    uint8_t * ptr = (uint8_t *)buffer;
    struct pseudo_terminal_master * ptm = file->priv;
    ASSERT(ptm);
    for (idx = 0; idx < size; idx++) {
        ptty_enqueue_byte(ptm, ptr[idx]);
    }
    if (ptm == current_ptty)
        ptty_flush_terminal(ptm);
    return size;
}

static int32_t
ptty_dev_size(struct file * file)
{
    return PTTY_ROWS * PTTY_COLUMNS;
}

static int32_t
ptty_dev_stat(struct file * file, struct stat * buff)
{
    buff->st_size = PTTY_ROWS * PTTY_COLUMNS;
    return OK;
}

static int32_t
ptty_dev_isatty(struct file * file)
{
    return 1;
}

static void
ptty_clear(struct pseudo_terminal_master * ptm)
{
    ptm->need_scroll = 0;
    ptm->row_front = 0;
    ptm->row_index = 0;
    ptm->col_index = 0;
    memset(ptm->buffer, 0x0, sizeof(ptm->buffer));
    if (current_ptty == ptm)
        reset_video_buffer();
}

static int32_t
ptty_dev_ioctl(struct file * file, uint32_t request, void * foo, void * bar)
{
    int32_t result = OK;
    struct pseudo_terminal_master * ptm = file->priv;
    switch (request)
    {
        case PTTY_IOCTL_CLEAR:
            ptty_clear(ptm);          
            break;
        case PTTY_IOCTL_MASTER:
            ptm->master_task_id = (uint32_t)foo;
            LOG_DEBUG("pseudo terminal:0x%x's master task set to:%d\n",
                ptm, ptm->master_task_id);
            break;
        case PTTY_IOCTL_FOREGROUND:
            ptm->foreground_task_id = (uint32_t)foo;
            ring_reset(&ptm->slave_ring);
            LOG_DEBUG("pseudo terminal:0x%x's slave task set to:%d\n",
                ptm, ptm->foreground_task_id);
            break;
        case PTTY_IOCTL_SLAVE_WRITE:
            {
                void * buffer = foo;
                uint32_t size = (uint32_t)bar;
                result = write_ring(&ptm->slave_ring, buffer, size);
            }
            break;
        default:
            result = -ERR_NOT_PRESENT;
            break;
    }
    return result;
}

static struct file_operation ptty_dev_ops = {
    .isatty = ptty_dev_isatty,
    .size = ptty_dev_size,
    .stat = ptty_dev_stat,
    .read = ptty_dev_read,
    .write = ptty_dev_write,
    .truncate = NULL,
    .ioctl = ptty_dev_ioctl
};

static void
disable_cursor(void)
{
    outb(CONSOLE_CURSOR_PORT0, 0x0a);
    outb(CONSOLE_CURSOR_PORT1, 0x20);
}

static void
update_cusor(int x, int y)
{
    uint16_t pos = y * PTTY_COLUMNS + x;
    outb(CONSOLE_CURSOR_PORT0, 0x0f);
    outb(CONSOLE_CURSOR_PORT1, (uint8_t)(pos & 0xff));
    outb(CONSOLE_CURSOR_PORT0, 0x0e);
    outb(CONSOLE_CURSOR_PORT1, (uint8_t)((pos >> 8) & 0xff));
}

static void
enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    outb(CONSOLE_CURSOR_PORT0, 0x0a);
    outb(CONSOLE_CURSOR_PORT1,
        (inb(CONSOLE_CURSOR_PORT1) & 0xc0) | cursor_start);
    outb(CONSOLE_CURSOR_PORT0, 0x0b);
    outb(CONSOLE_CURSOR_PORT1,
        (inb(CONSOLE_CURSOR_PORT1) & 0xe0) | cursor_end);
}

static void
switch_to_default_console(void * arg)
{
    expose_default_console();
    if (current_ptty) {
        ring_reset(&current_ptty->ring);
    }
    current_ptty = NULL;
    disable_cursor();
}

static void
switch_to_pseudo_terminal(void * arg)
{
    uint32_t ptty_index = (uint32_t)arg;
    ASSERT(ptty_index >= 0 && ptty_index < MAX_TERMINALS);
    hide_default_console();
    current_ptty = &ptties[ptty_index];
    current_ptty->need_scroll = 1;
    enable_cursor(0, 0);
    ptty_flush_terminal(current_ptty);
    ring_reset(&current_ptty->ring);
}

void
ptty_post_init(void)
{
    int idx = 0;
    uint8_t node_name[64];
    struct file_system * devfs = get_dev_filesystem();
    for (idx = 0; idx < MAX_TERMINALS; idx++) {
        ptty_init(&ptties[idx]);
        memset(node_name, 0x0, sizeof(node_name));
        sprintf((char *)node_name, "/ptm%d", idx);
        ASSERT(register_dev_node(devfs,
            node_name,
            0x0,
            &ptty_dev_ops,
            &ptties[idx]));
    }
    ASSERT(!register_shortcut_entry(SCANCODE_F1,
        KEY_STATE_ALT_PRESSED,
        switch_to_default_console,
        NULL));
    for (idx = 0; idx < MAX_TERMINALS; idx++) {
        ASSERT(!register_shortcut_entry(SCANCODE_F2 + idx,
            KEY_STATE_ALT_PRESSED,
            switch_to_pseudo_terminal,
            (void *)idx));
    }
    disable_cursor();
}
