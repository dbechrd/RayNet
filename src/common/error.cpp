#include "error.h"
#include "common.h"

const char *ErrStr(Err err)
{
    switch (err) {
        case RN_SUCCESS         : return "RN_SUCCESS        ";
        case RN_BAD_ALLOC       : return "RN_BAD_ALLOC      ";
        case RN_BAD_MAGIC       : return "RN_BAD_MAGIC      ";
        case RN_BAD_FILE_READ   : return "RN_BAD_FILE_READ  ";
        case RN_BAD_FILE_WRITE  : return "RN_BAD_FILE_WRITE ";
        case RN_INVALID_SIZE    : return "RN_INVALID_SIZE   ";
        case RN_INVALID_PATH    : return "RN_INVALID_PATH   ";
        case RN_NET_INIT_FAILED : return "RN_NET_INIT_FAILED";
        case RN_INVALID_ADDRESS : return "RN_INVALID_ADDRESS";
        case RN_RAYLIB_ERROR    : return "RN_RAYLIB_ERROR   ";
        case RN_BAD_ID          : return "RN_BAD_ID         ";
        case RN_OUT_OF_BOUNDS   : return "RN_OUT_OF_BOUNDS  ";
        default                 : return TextFormat("Code %d", err);
    }
}