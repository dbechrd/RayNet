#include "common.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"

Font fntHackBold20;
Font fntHackBold32;

Texture texLily;

Sound sndSoftTick;
Sound sndHardTick;

// TODO: Load placeholder textures/sounds etc. if fail
Err InitCommon(void)
{
    Err err = RN_SUCCESS;

    fntHackBold20 = LoadFontEx("resources/Hack-Bold.ttf", 20, 0, 0);
    if (!fntHackBold20.baseSize) err = RN_RAYLIB_ERROR;

    fntHackBold32 = LoadFontEx("resources/Hack-Bold.ttf", 32, 0, 0);
    if (!fntHackBold20.baseSize) err = RN_RAYLIB_ERROR;

    texLily = LoadTexture("resources/lily.png");
    if (!texLily.width) err = RN_RAYLIB_ERROR;

    sndSoftTick = LoadSound("resources/soft_tick.wav");
    if (!sndSoftTick.frameCount) err = RN_RAYLIB_ERROR;

    sndHardTick = LoadSound("resources/hard_tick.wav");
    if (!sndHardTick.frameCount) err = RN_RAYLIB_ERROR;

    return err;
}

void FreeCommon(void)
{
    UnloadFont(fntHackBold20);
    UnloadFont(fntHackBold32);
    UnloadTexture(texLily);
    UnloadSound(sndSoftTick);
    UnloadSound(sndHardTick);
}