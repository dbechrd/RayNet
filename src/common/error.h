#pragma once

enum Err {
    RN_SUCCESS          =  0,
    RN_BAD_ALLOC        = -1,
    RN_BAD_MAGIC        = -2,
    RN_BAD_FILE_READ    = -3,
    RN_BAD_FILE_WRITE   = -4,
    RN_INVALID_SIZE     = -5,
    RN_INVALID_PATH     = -6,
    RN_NET_INIT_FAILED  = -7,
    RN_INVALID_ADDRESS  = -8,
    RN_RAYLIB_ERROR     = -9,
    RN_BAD_ID           = -10,
    RN_OUT_OF_BOUNDS    = -11,
    RN_PARSE_ERROR      = -12,
};

#define ERR_RETURN(expr) \
    do { \
        Err err = expr; \
        if (err) return err; \
    } while(0);

#define ERR_RETURN_EX(expr, err) \
    if (!(expr)) { \
        return (err); \
    }

const char *ErrStr(Err err);