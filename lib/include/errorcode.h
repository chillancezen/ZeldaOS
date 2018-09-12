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
    ERR_PARTIAL
};

#endif
