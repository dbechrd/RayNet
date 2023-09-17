#pragma once
#include "common.h"

struct Manifold {
    Vector2 contact;
    Vector2 normal;
    float depth;
};

bool dlb_CheckCollisionPointRec(const Vector2 point, const Rectangle rec);
bool dlb_CheckCollisionCircleRec(const Vector2 center, const float radius, const Rectangle rec, Manifold *manifold);
bool dlb_CheckCollisionCircleRec2(const Vector2 center, const float radius, const Rectangle rec, Manifold *manifold);
