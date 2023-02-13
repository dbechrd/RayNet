/*******************************************************************************************
*
*   raylib [textures] example - Texture loading and drawing
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2023 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib/raylib.h"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int windowWidth = 1600;
    const int windowHeight = 900;

    InitWindow(windowWidth, windowHeight, "RayNet");
    SetWindowState(FLAG_VSYNC_HINT);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");        // Texture loading
    //---------------------------------------------------------------------------------------

    int fontSize = 32;
    Font font = LoadFontEx("resources/OpenSans-Bold.ttf", fontSize, 0, 0);

    const char *text = "Meow MEOW meow MeOw!";
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            windowWidth / 2 - texture.width / 2,
            windowHeight / 2 - texture.height / 2
        };
        DrawTexture(texture, catPos.x, catPos.y, WHITE);

        Vector2 textPos = {
            windowWidth / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        Vector2 textShadowPos = textPos;
        textShadowPos.x += 2;
        textShadowPos.y += 2;

        //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
        DrawTextEx(font, text, textShadowPos, fontSize, 1, BLACK);
        DrawTextEx(font, text, textPos, fontSize, 1, RAYWHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(font);
    UnloadTexture(texture);       // Texture unloading

    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}