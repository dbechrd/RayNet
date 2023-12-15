#include "boot_screen.h"

void DrawBootScreenMessage(const char *msg, int dots)
{
    BeginDrawing();
    ClearBackground(BLACK);
    Font font = GetFontDefault();
    const float fontSize = 48.0f;
    const float spacing = 4.0f;
    Vector2 msgSize = MeasureTextEx(font, msg, fontSize, spacing);
    Vector2 msgPos{
        GetRenderWidth() / 2.0f - msgSize.x / 2.0f,
        GetRenderHeight() / 2.0f - msgSize.y / 2.0f
    };
    DrawTextEx(font, msg, msgPos, fontSize, spacing, WHITE);

    if (dots) {
        Vector2 dotPos = msgPos;
        dotPos.x += msgSize.x + spacing;

        const char *dotBuf = "...";
        DrawTextEx(font, TextFormat("%.*s", CLAMP(dots, 1, 3), dotBuf), dotPos, fontSize, spacing, WHITE);
    }

    EndDrawing();
}

void DrawBootScreen(void)
{
    const char *msgs[]{
        "Dusting off the scripts",
        "Modernizing the humor",
        "Clearing the narrator's throat",
    };
    for (const char *msg : msgs) {
        for (int i = 0; i <= 3; i++) {
            DrawBootScreenMessage(msg, i);
            yojimbo_sleep(0.1);
        }
    }
}