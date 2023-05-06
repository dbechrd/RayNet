#include "shared_lib.h"
#include "audio/audio.cpp"
#include "collision.cpp"
#include "entity.cpp"
#include "file_utils.cpp"
#include "net/net.cpp"
#include "tilemap.cpp"
#include "wang.cpp"

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color)
{
#if 1
    Vector2 shadowPos = Vector2AddValue(pos, 2);
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