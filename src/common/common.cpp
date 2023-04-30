#include "common.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"

Font fntHackBold20;
Texture texYikes;

Err InitCommon(void)
{
    fntHackBold20 = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
    if (!fntHackBold20.baseSize) {
        return RN_RAYLIB_ERROR;
    }

    texYikes = LoadTexture("resources/lily.png");
    if (!texYikes.width) {
        return RN_RAYLIB_ERROR;
    }

    return RN_SUCCESS;
}

void FreeCommon(void)
{
    UnloadFont(fntHackBold20);
    UnloadTexture(texYikes);
}