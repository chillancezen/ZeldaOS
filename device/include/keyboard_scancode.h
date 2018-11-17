/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _KEYBOARD_SCANCODE_H
#define _KEYBOARD_SCANCODE_H
// The scan codes are from:
// https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Set_1

#define SCANCODE_C          0x2e
#define SCANCODE_BACKSPACE  0x0e
#define SCANCODE_F1         0x3b
#define SCANCODE_F2         (SCANCODE_F1 + 1)
#define SCANCODE_F3         (SCANCODE_F1 + 2)
#define SCANCODE_F4         (SCANCODE_F1 + 3)
#define SCANCODE_F5         (SCANCODE_F1 + 4)
#define SCANCODE_F6         (SCANCODE_F1 + 5)
#define SCANCODE_F7         (SCANCODE_F1 + 6)
#define SCANCODE_F8         (SCANCODE_F1 + 7)
#define SCANCODE_F9         (SCANCODE_F1 + 8)
#define SCANCODE_F10        (SCANCODE_F1 + 9)
#define SCANCODE_F11        0x57
#define SCANCODE_F12        0x58

#endif
