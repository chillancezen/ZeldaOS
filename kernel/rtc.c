/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/rtc.h>
#include <x86/include/ioport.h>


#define CMOS_REGISTER_SECOND 0x0
#define CMOS_REGISTER_MINUTE 0x2
#define CMOS_REGISTER_HOUR 0x4
#define CMOS_REGISTER_WEEKDAY 0x6
#define CMOS_REGISTER_DAY_OF_MONTH 0x7
#define CMOS_REGISTER_MONTH 0x8
#define CMOS_REGISTER_YEAR 0x9

static uint8_t
read_cmos(uint8_t register_number)
{
    register_number &= 0x7f;
    outb(CMOS_ADDRESS_PORT, register_number);
    return inb(CMOS_DATA_PORT);
}

void
read_rtc_datetime(struct rtc_datetime * datetime)
{
    datetime->second = read_cmos(CMOS_REGISTER_SECOND);
    datetime->minute = read_cmos(CMOS_REGISTER_MINUTE);
    datetime->hour = read_cmos(CMOS_REGISTER_HOUR);
    datetime->month = read_cmos(CMOS_REGISTER_MONTH);
    datetime->year = read_cmos(CMOS_REGISTER_YEAR);
    datetime->weekday = read_cmos(CMOS_REGISTER_WEEKDAY);
    datetime->day_of_month = read_cmos(CMOS_REGISTER_DAY_OF_MONTH);
}
