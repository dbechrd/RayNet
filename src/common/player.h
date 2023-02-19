#pragma once
#include "common.h"

struct Player {
    Color color;
    Vector2 size;

    float speed;
    Vector2 velocity;
    Vector2 position;

    void Draw(void);
};