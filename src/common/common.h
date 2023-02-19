#pragma once

#include "raylib/raylib.h"
#include "yojimbo.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 900

#define FONT_PATH "resources/Hack-Bold.ttf"
#define FONT_SIZE 20

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, float fontSize, Color color);
