#pragma once
#include <queue>

struct InputCommand {
    int  seq;
    bool north;
    bool west;
    bool south;
    bool east;
};

typedef std::queue<InputCommand> InputCommandQueue;