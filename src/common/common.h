#pragma once

#include "raylib/raylib.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 900

#define FONT_PATH "resources/Hack-Bold.ttf"
#define FONT_SIZE 20

struct Player {
    Color color;
    Vector2 size;

    float speed;
    Vector2 velocity;
    Vector2 position;

    void Draw(void) {
        DrawRectanglePro(
            { position.x, position.y, size.x, size.y },
            { size.x / 2, size.y },
            0,
            color
        );
    }
};

struct World {
    Player player;
};

extern World *g_world;

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, float fontSize, Color color);
