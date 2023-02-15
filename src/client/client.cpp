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
static double clientTime = 100.0;

yojimbo::Client &ClientStart()
{
    static yojimbo::Client *client = nullptr;
    if (client) {
        return *client;
    }

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::ClientServerConfig config;

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, adapter, clientTime);

    yojimbo::Address serverAddress("127.0.0.1", ServerPort);

#if 0
    if (argc == 2)
    {
        Address commandLineAddress(argv[1]);
        if (commandLineAddress.IsValid())
        {
            if (commandLineAddress.GetPort() == 0)
                commandLineAddress.SetPort(ServerPort);
            serverAddress = commandLineAddress;
        }
    }
#endif

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    client->InsecureConnect(privateKey, clientId, serverAddress);

    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);

    return *client;
}

void ClientUpdate(yojimbo::Client &client)
{
    static double lastSentAt = 0;
    if (clientTime > lastSentAt + 0.2) {
        TestMessage *message = (TestMessage *)client.CreateMessage(TEST_MESSAGE);
        if (message)
        {
            static uint16_t numMessagesSentToServer = 0;
            message->sequence = (uint16_t)numMessagesSentToServer;
            message->hitpoints = 100 - numMessagesSentToServer;
            client.SendMessage(0, message);
            numMessagesSentToServer++;
            lastSentAt = clientTime;
        }
    }

    client.SendPackets();
    client.ReceivePackets();

    if (client.IsDisconnected()) {
        return;
    }

    clientTime += deltaTime;
    client.AdvanceTime(clientTime);

    if (client.ConnectionFailed())
        return;

    yojimbo_sleep(deltaTime);
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(windowWidth, windowHeight, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2, // - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");        // Texture loading
    //---------------------------------------------------------------------------------------

    int fontSize = 32;
    Font font = LoadFontEx("resources/OpenSans-Bold.ttf", fontSize, 0, 0);

    const char *text = "Meow MEOW meow MeOw!";
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

    yojimbo::Client &client = ClientStart();
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        ClientUpdate(client);

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

    ClientStop(client);

    delete &client;

    ShutdownYojimbo();
    //--------------------------------------------------------------------------------------

    return 0;
}