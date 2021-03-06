/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H
#include <lib/include/types.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_COMMAND_PORT 0x64

#define KEYBOARD_LED_SCROLLLOCK 0x0
#define KEYBOARD_LED_NUMBERLOCK 0x1
#define KEYBOARD_LED_CAPSLOCK 0x2

#define SCROLLLOCK_IS_ON (1 << KEYBOARD_LED_SCROLLLOCK)
#define NUMBERLOCK_IS_ON (1 << KEYBOARD_LED_NUMBERLOCK)
#define CAPSLOCK_IS_ON (1 << KEYBOARD_LED_CAPSLOCK)

#define KEY_STATE_PRESSED 0x1 
#define KEY_STATE_CONTROLL_PRESSED 0x2
#define KEY_STATE_SHIFT_PRESSED 0x4
#define KEY_STATE_ALT_PRESSED 0x8
#define KEY_STATE_CAPS_LOCKED 0x10


void keyboard_init(void);

#define SCANCODE_CONTROL 0x1d
#define SCANCODE_LSHIFT 0x2a
#define SCANCODE_RSHIFT 0x36
#define SCANCODE_ALT 0x38
#define SCANCODE_CAPS 0x3a

struct keyboard_shortcut_entry {
    uint8_t scancode;
    uint8_t keystate;
    void (*handler)(void *);
    void * arg;
};

int32_t register_shortcut_entry(uint8_t scancode,
    uint8_t keystate,
    void (*handler)(void*),
    void * arg);

#endif
