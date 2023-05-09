#include "shared_lib.h"
#include "audio/audio.cpp"
#include "collision.cpp"
#include "editor.cpp"
#include "entity.cpp"
#include "file_utils.cpp"
#include "net/net.cpp"
#include "tilemap.cpp"
#include "wang.cpp"

float GetRandomFloatZeroToOne(void)
{
    return (float)rand() / RAND_MAX;
}

float GetRandomFloatMinusOneToOne(void)
{
    return GetRandomFloatZeroToOne() * 2.0f - 1.0f;
}

float GetRandomFloatVariance(float variance)
{
    return GetRandomFloatMinusOneToOne() * variance;
}

void RandTests(void)
{
    float aMin = FLT_MAX;
    float aMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatZeroToOne();
        aMin = MIN(aMin, f);
        aMax = MAX(aMax, f);
    }
    printf("GetRandomFloatZeroToOne: %.2f - %.2f\n", aMin, aMax);

    float bMin = FLT_MAX;
    float bMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatMinusOneToOne();
        bMin = MIN(bMin, f);
        bMax = MAX(bMax, f);
    }
    printf("GetRandomFloatMinusOneToOne: %.2f - %.2f\n", bMin, bMax);

    float cMin = FLT_MAX;
    float cMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatVariance(0.2f);
        cMin = MIN(cMin, f);
        cMax = MAX(cMax, f);
    }
    printf("GetRandomFloatVariance(0.2f): %.2f - %.2f\n", cMin, cMax);
}

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color)
{
#if 1
    Vector2 shadowPos = Vector2Add(pos, { 1, 1 });
    if (font.baseSize > 32) {
        shadowPos = Vector2AddValue(pos, 2);
    }

    //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
    if (color.r || color.g || color.b) {
        // Don't draw shadows on black text
        DrawTextEx(font, text, shadowPos, font.baseSize, 1, BLACK);
    }
    DrawTextEx(font, text, pos, font.baseSize, 1, color);
#else
    DrawTextEx(font, text, pos, font.baseSize, 1, color);
#endif
}

Rectangle GetScreenRectWorld(Camera2D &camera)
{
#if CL_DBG_TILE_CULLING
    const int screenMargin = 64;
    Vector2 screenTLWorld = GetScreenToWorld2D({ screenMargin, screenMargin }, camera);
    Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth() - screenMargin, (float)GetScreenHeight() - screenMargin }, camera);
#else
    const Vector2 screenTLWorld = GetScreenToWorld2D({ 0, 0 }, camera);
    const Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera);
#endif

    const Rectangle rectWorld{
        screenTLWorld.x,
        screenTLWorld.y,
        screenBRWorld.x - screenTLWorld.x,
        screenBRWorld.y - screenTLWorld.y,
    };
    return rectWorld;
}

#ifdef TRACY_ENABLE
#include "tracy/tracy/Tracy.hpp"

void *operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void *ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif