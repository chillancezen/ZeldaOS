/*
 * Copyright (c) 2018 Jie Zheng
 * 
 *
 * The keyboad module code only process the standard scancode of set1
 * , i.e. US QWERTY keyboard layout is recoginized.
 */

#include <device/include/keyboard.h>
#include <device/include/keyboard_scancode.h>
#include <x86/include/interrupt.h>
#include <kernel/include/printk.h>
#include <x86/include/ioport.h>
#include <lib/include/string.h>
#include <lib/include/errorcode.h>

#define KEYBOARD_INTERRUPT_VECTOR (0x20 + 1)
#define KEY_SPACE_SIZE 256
/*
 * The global shortcut space size
 */

#define MAX_SHORTCUT_SIZE 256

static uint8_t key_state = 0;
static uint8_t keycode[KEY_SPACE_SIZE]; //scan code to ascii
static uint8_t keyshift[KEY_SPACE_SIZE]; //ascii conversion table
static struct keyboard_shortcut_entry shortcut_entries[MAX_SHORTCUT_SIZE];
static int shortcut_entry_ptr = 0;

void __switch_led(uint8_t mask)
{
    outb(KEYBOARD_COMMAND_PORT, 0xed);
    __asm__ volatile("pause;");
    outb(KEYBOARD_COMMAND_PORT, mask);
}

uint8_t retrieve_scancode(void)
{
    uint8_t code = 0;
    do {
        code = inb(KEYBOARD_DATA_PORT);
        if (code)
            break;
    } while(1);
    return code;
}
/*
 * The function does not process any extended scancode
 * like multimedia scancode
 */

uint8_t process_scancode(uint8_t scancode)
{
    /*
     * check whether the Key is pressed or released
     */
    if (scancode & 0x80) {
        key_state &= ~KEY_STATE_PRESSED;
    }else {
        key_state |= KEY_STATE_PRESSED;
    }
    scancode = scancode & 0x7f;
    /*
     * check and update the control/shift/alt  and caps key status
     */
    if (key_state & KEY_STATE_PRESSED) {
        switch (scancode)
        {
            case SCANCODE_CONTROL:
                key_state |= KEY_STATE_CONTROLL_PRESSED;
                break;
            case SCANCODE_LSHIFT:
            case SCANCODE_RSHIFT:
                key_state |= KEY_STATE_SHIFT_PRESSED;
                break;
            case SCANCODE_ALT:
                key_state |= KEY_STATE_ALT_PRESSED;
                break;
            case SCANCODE_CAPS:
                key_state = (key_state & KEY_STATE_CAPS_LOCKED) ?
                    key_state & ~KEY_STATE_CAPS_LOCKED:
                    key_state | KEY_STATE_CAPS_LOCKED;
                break;
            default:
                break;
        }
    } else {
        switch (scancode)
        {
            case SCANCODE_CONTROL:
                key_state &= ~KEY_STATE_CONTROLL_PRESSED;
                break;
            case SCANCODE_LSHIFT:
            case SCANCODE_RSHIFT:
                key_state &= ~KEY_STATE_SHIFT_PRESSED;
                break;
            case SCANCODE_ALT:
                key_state &= ~KEY_STATE_ALT_PRESSED;
                break;
            default:
                break;
        }
    }
    return scancode;
}

uint8_t to_ascii(uint8_t scancode, uint8_t keystate)
{

    uint8_t __ascii_code = keycode[scancode];
    if (__ascii_code == 0xff)
        return __ascii_code;
    //to upper case
    if (__ascii_code >= 'a' &&
        __ascii_code <= 'z' &&
        ((keystate & KEY_STATE_CAPS_LOCKED) ||
        (keystate & KEY_STATE_SHIFT_PRESSED))) {
        __ascii_code += 'A' -'a';
    } else if (keystate & KEY_STATE_SHIFT_PRESSED) {
        if (keyshift[__ascii_code] != 0xff ) {
            __ascii_code = keyshift[__ascii_code];
        }
    }
    return __ascii_code;
}

static void
hook_scancode(uint8_t scancode, uint8_t keystate)
{
    int idx = 0;
    for (idx = 0; idx < shortcut_entry_ptr; idx++) {
        if (shortcut_entries[idx].scancode == scancode && 
            shortcut_entries[idx].keystate == keystate)
            break;
    }
    if(idx < shortcut_entry_ptr && shortcut_entries[idx].handler)
        shortcut_entries[idx].handler(shortcut_entries[idx].arg);
}

void keyboard_interrupt_handler(struct interrupt_argument * parg __used)
{
    uint8_t scancode = retrieve_scancode();
    uint8_t asciicode;
    scancode = process_scancode(scancode);
    hook_scancode(scancode, key_state);
    asciicode = to_ascii(scancode, key_state);

    if (key_state & KEY_STATE_PRESSED && asciicode != 0xff){
        //printk("%c", asciicode); 
    }
}

int32_t register_shortcut_entry(uint8_t scancode,
    uint8_t keystate,
    void (*handler)(void*),
    void * arg)
{
    //Enforce key is pressed
    keystate |= KEY_STATE_PRESSED;
    int idx = 0;
    for (idx = 0; idx < shortcut_entry_ptr; idx++) {
        if(shortcut_entries[idx].scancode == scancode &&
            shortcut_entries[idx].keystate == keystate)
            break;
    }
    if (idx < shortcut_entry_ptr){
        shortcut_entries[idx].handler = handler;
        shortcut_entries[idx].arg = arg;
        return OK;
    }
    if (shortcut_entry_ptr == MAX_SHORTCUT_SIZE)
        return -ERR_OUT_OF_RESOURCE;
    shortcut_entries[shortcut_entry_ptr].scancode = scancode;
    shortcut_entries[shortcut_entry_ptr].keystate = keystate;
    shortcut_entries[shortcut_entry_ptr].handler = handler;
    shortcut_entries[shortcut_entry_ptr].arg = arg;
    shortcut_entry_ptr++;
    return OK;
}

void CTRL_ALT_DELETE(void * arg __used)
{
    printk("%x\n", &arg);
    dump_registers();
}

void
keyboard_init(void)
{
    int idx = 0;
    for (; idx < KEY_SPACE_SIZE; idx++) {
        keycode[idx] = 0xff;
        keyshift[idx] = 0xff;
    }
    memset(shortcut_entries, 0x0, sizeof(shortcut_entries));
    
    register_shortcut_entry(SCANCODE_DELETE,
        KEY_STATE_CONTROLL_PRESSED|KEY_STATE_ALT_PRESSED,
        CTRL_ALT_DELETE,
        NULL);
    /*
     * please refer to https://en.wikipedia.org/wiki/ASCII
     */
    //keycode[0x01] = '\x1b'; //escape key
    keycode[0x02] = '1';
    keycode[0x03] = '2';
    keycode[0x04] = '3';
    keycode[0x05] = '4';
    keycode[0x06] = '5';
    keycode[0x07] = '6';
    keycode[0x08] = '7';
    keycode[0x09] = '8';
    keycode[0x0a] = '9';
    keycode[0x0b] = '0';
    keycode[0x0c] = '-';
    keycode[0x0d] = '=';
    //keycode[0x0e] = '\x7f'; //backspace&delete key
    //keycode[0x0f] = '\t'; //horizontal TAB key
    keycode[0x10] = 'q';
    keycode[0x11] = 'w';
    keycode[0x12] = 'e';
    keycode[0x13] = 'r';
    keycode[0x14] = 't';
    keycode[0x15] = 'y';
    keycode[0x16] = 'u';
    keycode[0x17] = 'i';
    keycode[0x18] = 'o';
    keycode[0x19] = 'p';
    keycode[0x1a] = '[';
    keycode[0x1b] = ']';
    keycode[0x1c] = '\n'; //line feed & enter
    keycode[0x1e] = 'a';
    keycode[0x1f] = 's';
    keycode[0x20] = 'd';
    keycode[0x21] = 'f';
    keycode[0x22] = 'g';
    keycode[0x23] = 'h';
    keycode[0x24] = 'j';
    keycode[0x25] = 'k';
    keycode[0x26] = 'l';
    keycode[0x27] = ';';
    keycode[0x28] = '\'';
    keycode[0x29] = '`';
    keycode[0x2b] = '\\';
    keycode[0x2c] = 'z';
    keycode[0x2d] = 'x';
    keycode[0x2e] = 'c';
    keycode[0x2f] = 'v';
    keycode[0x30] = 'b';
    keycode[0x31] = 'n';
    keycode[0x32] = 'm';
    keycode[0x33] = ',';
    keycode[0x34] = '.';
    keycode[0x35] = '/';
    keycode[0x39] = ' ';

    keyshift['`'] = '~';
    keyshift['1'] = '!';
    keyshift['2'] = '@';
    keyshift['3'] = '#';
    keyshift['4'] = '$';
    keyshift['5'] = '%';
    keyshift['6'] = '^';
    keyshift['7'] = '&';
    keyshift['8'] = '*';
    keyshift['9'] = '(';
    keyshift['0'] = ')';
    keyshift['-'] = '_';
    keyshift['='] = '+';
    keyshift['['] = '{';
    keyshift[']'] = '}';
    keyshift['\\'] = '|';
    keyshift[';'] = ':';
    keyshift['\''] = '"';
    keyshift[','] = '<';
    keyshift['.'] = '>';
    keyshift['/'] = '?';

    register_interrupt_handler(KEYBOARD_INTERRUPT_VECTOR,
        keyboard_interrupt_handler,
        "PS/2 keyboard");
}
