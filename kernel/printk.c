/*
 *Copyright (c) 2018 Jie Zheng
 */
#include <stdarg.h>
#include <kernel/include/printk.h>

#if defined(SERIAL_OUTPUT)
    #include <device/include/serial.h>
#endif
#define VGA_MAX_ROW 25
#define VGA_MAX_COL 80
#define VGA_MEMORY_BASE 0xb8000

int __log_level = LOG_DEBUG;

static int vga_row_front = 0;
static int vga_row_idx = 0;
static int vga_col_idx = 0;

static uint8_t vga_shadow_memory[VGA_MAX_ROW+1][VGA_MAX_COL];
static uint16_t (*vga_ptr)[VGA_MAX_COL] = (void *)VGA_MEMORY_BASE;
static int default_console_hidden = 0;

void
reset_video_buffer(void)
{
    int idx_row;
    int idx_col;
    for (idx_row = 0; idx_row < VGA_MAX_ROW; idx_row++)
        for(idx_col = 0; idx_col < VGA_MAX_COL; idx_col++)
            vga_ptr[idx_row][idx_col] &= 0xff00;
}

void
hide_default_console(void)
{
    default_console_hidden = 1;
    reset_video_buffer();
}
void
expose_default_console(void)
{
    default_console_hidden = 0;
    reset_video_buffer();
    printk_flush();
}
void
set_log_level(int level)
{
    __log_level = level;
}

void
printk_init()
{
    int idx_row = 0;
    int idx_col = 0;
    for (idx_row = 0; idx_row < VGA_MAX_ROW + 1; idx_row++)
        for (idx_col = 0; idx_col < VGA_MAX_COL; idx_col++)
            vga_shadow_memory[idx_row][idx_col] = 0;
    set_log_level(KERNEL_LOGGING_LEVEL);
}
int
vga_enqueue_byte(uint8_t target,
    void * opaque0 __used,
    void * opaque1 __used)
{
    int idx = 0;
#if defined(SERIAL_OUTPUT)
    write_serial(target);
#endif
    if (target != '\n')
        vga_shadow_memory[vga_row_idx][vga_col_idx] = target;
    vga_col_idx ++;
    if (vga_col_idx == VGA_MAX_COL || target == '\n') {
        vga_col_idx = 0;
        vga_row_idx = (vga_row_idx + 1) % (VGA_MAX_ROW + 1);
        if ((vga_row_idx + 1) % (VGA_MAX_ROW + 1) == vga_row_front) {
            for(idx = 0; idx < VGA_MAX_COL; idx++)
                vga_shadow_memory[vga_row_front][idx] = 0;
            vga_row_front = (vga_row_front + 1) % (VGA_MAX_ROW + 1);
        }
    }
    return 0;
}

void printk_flush()
{
    int real_row_idx = 0;
    int idx = vga_row_front;
    int tmp_idx;
    if (default_console_hidden)
        return;
    for (; (vga_row_idx + 1) % (VGA_MAX_ROW + 1) != idx;
        idx = (idx + 1) % (VGA_MAX_ROW + 1), real_row_idx++){
        for (tmp_idx = 0; tmp_idx < VGA_MAX_COL; tmp_idx++)
            vga_ptr[real_row_idx][tmp_idx] = 
                (vga_ptr[real_row_idx][tmp_idx] & 0xff00) |
                vga_shadow_memory[idx][tmp_idx];
    }
}
#define DEFAULT_STACK_SIZE 64
static void
resolve_decimal(int32_t val,
    int (*func)(uint8_t, void *, void *),
    void * opaque0,
    void * opaque1)
{
    int8_t stack[DEFAULT_STACK_SIZE];
    int iptr = 0;
    int8_t mod;
    int8_t null_flag = 1;
    if (val < 0)
        func('-', opaque0, opaque1);
    val = (val < 0) ? -val : val;
    while(val && iptr < DEFAULT_STACK_SIZE) {
        null_flag = 0;
        mod = val % 10;
        stack[iptr++] = mod + '0';
        val = val / 10;
    }
    if (null_flag) {
        stack[iptr++] = '0';
    }
    while(iptr > 0) {
        func(stack[--iptr], opaque0, opaque1);
    }
}
static void
resolve_hexdecimal(int32_t _val,
    int lowcase,
    int (*func)(uint8_t, void *, void *),
    void * opaque0,
    void * opaque1)
{
    uint8_t lower[] = "0123456789abcdef";
    uint8_t upper[] = "0123456789ABCDEF";
    uint8_t stack[DEFAULT_STACK_SIZE];
    int iptr = 0;
    int32_t  mod;
    int64_t val = _val & 0x0ffffffffl;
    int8_t null_flag = 1;
    while(val && iptr < DEFAULT_STACK_SIZE) {
        null_flag = 0;
        mod = val & 0xf;
        stack[iptr++] = lowcase ? lower[mod] : upper[mod];
        val = val >> 4;
    }
    if (null_flag) {
        stack[iptr++] = '0';
    }
    while(iptr > 0) {
        func(stack[--iptr], opaque0, opaque1);
    }
}

static int
sprintf_lambda_func(uint8_t c, void * opaque0, void * opaque1)
{
    uint8_t * dst = (uint8_t *)opaque0;
    int32_t * pindex = (int32_t *)opaque1;
    dst[*pindex] = c;
    *pindex += 1;
    return 0;
}
int
sprintf(char * dst, const char * format, ...)
{
    int ret = 0;
    const char * ptr = format;
    char * str_arg;
    int int_arg;
    int _iptr;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    for(; *ptr; ptr++) {
        if(*ptr != '%') {
            dst[ret++] = *ptr;
        } else {
            ++ptr;
            switch(*ptr)
            {
                case 's':
                    str_arg = va_arg(arg_ptr, char *);
                    for (_iptr = 0; str_arg[_iptr]; _iptr++)
                        dst[ret++] = str_arg[_iptr];  
                    break;
                case 'c':
                    int_arg = va_arg(arg_ptr, int);
                    dst[ret++] = (char)int_arg;
                    break;
                case 'd':
                case 'h':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_decimal(int_arg, sprintf_lambda_func,
                        dst, &ret);
                    break;
                case 'x':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 1, sprintf_lambda_func,
                        dst, &ret);
                    break;
                case 'X':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, sprintf_lambda_func,
                        dst, &ret);
                    break;
                case 'l':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, sprintf_lambda_func,
                        dst, &ret);
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, sprintf_lambda_func,
                        dst, &ret);
                    break;
                default:
                    break;

            }
        }

    }
    dst[ret++] = '\x0';
    return ret; 
}
void
printk(const char * format, ...)
{
    const char * ptr = format;
    char * str_arg;
    int int_arg;
    int _iptr;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    for(; *ptr; ptr++) {
        if (*ptr != '%') {
            vga_enqueue_byte(*ptr, NULL, NULL);
        } else {
            ++ptr;
            switch(*ptr)
            {
                case 's':
                    str_arg = va_arg(arg_ptr, char *);
                    for (_iptr = 0; str_arg[_iptr]; _iptr++)
                        vga_enqueue_byte(str_arg[_iptr], NULL, NULL);
                    break;
                case 'c': //as a character
                    int_arg = va_arg(arg_ptr, int);
                    vga_enqueue_byte((char)int_arg, NULL, NULL);
                    break;
                case 'd':
                case 'h'://as a short in decimal format
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_decimal(int_arg, vga_enqueue_byte, NULL, NULL);
                    break;
                case 'x':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 1, vga_enqueue_byte, NULL, NULL);
                    break;
                case 'X':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, vga_enqueue_byte, NULL, NULL);
                    break;
                case 'l':
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, vga_enqueue_byte, NULL, NULL);
                    int_arg = va_arg(arg_ptr, int32_t);
                    resolve_hexdecimal(int_arg, 0, vga_enqueue_byte, NULL, NULL);
                    break;
            }

        }
    }
    va_end(arg_ptr);
    printk_flush();
}
