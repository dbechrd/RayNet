#pragma once
#include "common.h"
#include "spritesheet.h"

struct Sprite {
    SpritesheetId spritesheetId;
    AnimationId animationId;
    int frame;
    double frameStartedAt;

    Vector2 GetSize(void);
    void Draw(Vector2 pos, float scale, double now);
};