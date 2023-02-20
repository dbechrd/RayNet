#pragma once
#include "common.h"

struct Entity {
    Color color;
    Vector2 size;

    float speed;
    Vector2 velocity;
    Vector2 position;

    void Draw(const Font &font, int clientIdx);
};