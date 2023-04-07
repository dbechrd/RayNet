#pragma once
#include "../common.h"

enum EntityType;

struct EntitySnapshot {
    double      serverTime {};
    uint32_t    lastProcessedInputCmd {};

    uint32_t    id         {};
    EntityType  type       {};
    Color       color      {};
    Vector2     size       {};
    float       speed      {};
    Vector2     velocity   {};
    Vector2     position   {};
};
