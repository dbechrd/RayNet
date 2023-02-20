#pragma once
#include "common.h"

struct EntityState {
    double  serverTime {};
    Color   color      {};
    Vector2 size       {};
    float   speed      {};
    Vector2 velocity   {};
    Vector2 position   {};
};