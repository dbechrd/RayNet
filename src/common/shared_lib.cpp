#include "shared_lib.h"
#include "audio/audio.cpp"
#include "collision.cpp"
#include "editor.cpp"
#include "entity.cpp"
#include "file_utils.cpp"
#include "net/net.cpp"
#include "tilemap.cpp"
#include "wang.cpp"

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color)
{
#if 1
    Vector2 shadowPos = Vector2Add(pos, { 1, 2 });
    if (font.baseSize > 32) {
        shadowPos = Vector2AddValue(pos, 2);
    }

    //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
    DrawTextEx(font, text, shadowPos, font.baseSize, 1, BLACK);
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