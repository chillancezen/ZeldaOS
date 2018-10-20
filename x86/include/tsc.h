/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _TSC_H
#define _TSC_H
// In Linux, we mostly use jiffies to meaure the elapsed time.
// This is inaccurent to calculate sometimes, e.g. if Timer interrupt is 
// pending too much, The jiffies will lag alot.
// In Intel X86 CPU, we assume it support TSC which is maintained by processor's
// pipeline, it's supposed to be highly precise.

#include <lib/include/types.h>
// TODO

#endif
