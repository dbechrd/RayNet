#pragma once
#include "common.h"
#include "strings.h"

struct AspectSprite {
    StringId spritesheetId;
    StringId animationId;
    int frame;
    double frameStartedAt;

    Vector2 GetSize(void);
    void Draw(Vector2 pos, float scale, double now);
};