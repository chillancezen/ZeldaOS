/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ERRORCODE_H
#define _ERRORCODE_H

enum errorcode {
    OK = 0,
    ERR_GENERIC,
    ERR_OUT_OF_MEMORY,
    ERR_OUT_OF_RESOURCE,
    ERR_INVALID_ARG,
    ERR_DEVICE_FAULT,
    ERR_PARTIAL,
    ERR_NOT_PRESENT,
    ERR_NOT_FOUND,
    ERR_NOT_SUPPORTED,
    ERR_BUSY,
    ERR_EXIST,
    ERR_IN_USE,
    ERR_INTERRUPTED,
};

#endif
