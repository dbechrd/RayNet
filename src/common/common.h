#pragma once
#include "raylib/raylib.h"
#include "yojimbo.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 900

#define FONT_PATH "resources/Hack-Bold.ttf"
#define FONT_SIZE 20

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))