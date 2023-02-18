#include "common.h"

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, float fontSize, Color color)
{
    Vector2 shadowPos = pos;
    shadowPos.x += 2;
    shadowPos.y += 2;

    //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
    DrawTextEx(font, text, shadowPos, fontSize, 1, BLACK);
    DrawTextEx(font, text, pos, fontSize, 1, color);
}