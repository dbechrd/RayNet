#include "shared_lib.h"
#include "collision.cpp"
#include "entity.cpp"
#include "net/net.cpp"
#include "tilemap.cpp"

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, float fontSize, Color color)
{
    Vector2 shadowPos = pos;
    shadowPos.x += 1;
    shadowPos.y += 1;

    //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
    DrawTextEx(font, text, shadowPos, fontSize, 1, BLACK);
    DrawTextEx(font, text, pos, fontSize, 1, color);
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