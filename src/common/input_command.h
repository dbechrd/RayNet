#pragma once
#include "common.h"

struct InputCommand {
    uint32_t seq;
    bool north;
    bool west;
    bool south;
    bool east;
};

struct InputCommandQueue {
    int nextIdx;
    InputCommand data[CLIENT_SEND_INPUT_COUNT];
};
