#include "boot_screen.h"

void DrawBootScreen(void)
{
    BeginDrawing();
    ClearBackground(BLACK);
    std::string loadingText = "Loading...";
    Vector2 loadingTextSize = MeasureTextEx(GetFontDefault(), loadingText.c_str(), 96.0f, 4.0f);
    DrawTextEx(
        GetFontDefault(),
        loadingText.c_str(),
        {
            GetRenderWidth() / 2.0f - loadingTextSize.x / 2.0f,
            GetRenderHeight() / 2.0f - loadingTextSize.y / 2.0f
        },
        96.0f,
        4.0f,
        WHITE
    );
    EndDrawing();
}