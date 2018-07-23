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

void keyboard_init(void);

#endif
