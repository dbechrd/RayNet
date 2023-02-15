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

#include "../common/common.h"

static TestAdapter adapter;
static double serverTime = 100.0;

yojimbo::Server &ServerStart()
{
    static yojimbo::Server *server = nullptr;
    if (server) {
        return *server;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    yojimbo::ClientServerConfig config;

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", ServerPort), config, adapter, serverTime);

    server->Start(yojimbo::MaxClients);
    if (!server->IsRunning()) {
        printf("yj: Failed to start server\n");
        return *server;
    }

    printf("yj: started server on port %d (insecure)\n", ServerPort);
    char addressString[256];
    server->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: server address is %s\n", addressString);

    return *server;
}

void ServerUpdate(yojimbo::Server &server)
{
    if (!server.IsRunning())
        return;

    server.SendPackets();
    server.ReceivePackets();

    const int clientIdx = 0;
    const int channelIdx = 0;
    yojimbo::Message *message = server.ReceiveMessage(clientIdx, channelIdx);
    do {
        if (message) {
            switch (message->GetType())
            {
                case TEST_MESSAGE:
                {
                    TestMessage *testMessage = (TestMessage *)message;
                    printf("yj: recv[seq=%d] TEST_MESSAGE (hp=%d)\n",
                        testMessage->sequence,
                        testMessage->hitpoints
                    );
                    server.ReleaseMessage(clientIdx, message);
                }
                break;
            }
        }
        message = server.ReceiveMessage(clientIdx, channelIdx);
    } while (message);

    serverTime += deltaTime;
    server.AdvanceTime(serverTime);

    yojimbo_sleep(deltaTime);
}

void ServerStop(yojimbo::Server &server)
{
    server.Stop();
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(windowWidth, windowHeight, "RayNet Server");
    SetWindowState(FLAG_VSYNC_HINT);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x, // / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");        // Texture loading
    //---------------------------------------------------------------------------------------

    int fontSize = 32;
    Font font = LoadFontEx("resources/OpenSans-Bold.ttf", fontSize, 0, 0);

    const char *text = "Listening...";
    Vector2 textSize = MeasureTextEx(font, text, (float)fontSize, 1);

    //---------------------------------------------------------------------------------------
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return 1;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);
    // yojimbo does this.. why?
    //srand((unsigned int)time(NULL));

    yojimbo::Server &server = ServerStart();
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        ServerUpdate(server);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            windowWidth / 2.0f - texture.width / 2.0f,
            windowHeight / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        Vector2 textPos = {
            windowWidth / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        Vector2 textShadowPos = textPos;
        textShadowPos.x += 2;
        textShadowPos.y += 2;

        //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
        DrawTextEx(font, text, textShadowPos, (float)fontSize, 1, BLACK);
        DrawTextEx(font, text, textPos, (float)fontSize, 1, RAYWHITE);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadFont(font);
    UnloadTexture(texture);       // Texture unloading

    CloseWindow();                // Close window and OpenGL context

    ServerStop(server);

    delete &server;

    ShutdownYojimbo();
    //--------------------------------------------------------------------------------------

    return 0;
}