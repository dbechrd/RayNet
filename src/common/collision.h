#pragma once
#include "common.h"

struct Manifold {
    Vector2 contact;
    Vector2 normal;
    float depth;
};

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec);
bool dlb_CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec, Manifold *manifold);