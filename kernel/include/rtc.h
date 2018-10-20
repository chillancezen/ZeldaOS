/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _RTC_H
#define _RTC_H
// Reference Document:
// https://wiki.osdev.org/CMOS
// https://wiki.osdev.org/RTC
//
#include <lib/include/types.h>

#define CMOS_ADDRESS_PORT 0x70
#define CMOS_DATA_PORT 0x71

struct rtc_datetime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t month;
    uint8_t year;
    uint8_t weekday;
    uint8_t day_of_month;
};



void
read_rtc_datetime(struct rtc_datetime * datetime);

#endif
