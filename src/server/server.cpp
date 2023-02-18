#include "../common/common.h"

TestAdapter adapter{};

yojimbo::Server &ServerStart()
{
    static yojimbo::Server *server = nullptr;
    if (server) {
        return *server;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    yojimbo::ClientServerConfig config{};

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", ServerPort), config, adapter, 100);

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

    server.AdvanceTime(server.GetTime() + deltaTime);

    //yojimbo_sleep(deltaTime);
}

void ServerStop(yojimbo::Server &server)
{
    server.Stop();
}

int main(void)
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
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

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    const char *text = "Listening...";
    Vector2 textSize = MeasureTextEx(font, text, (float)FONT_SIZE, 1);

    //--------------------
    // Server
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

    while (!WindowShouldClose())
    {
        //--------------------
        // Update
        ServerUpdate(server);

        //--------------------
        // Draw
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            WINDOW_WIDTH / 2.0f - texture.width / 2.0f,
            WINDOW_HEIGHT / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        Vector2 textPos = {
            WINDOW_WIDTH / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        DrawTextShadowEx(font, text, textPos, (float)FONT_SIZE, RAYWHITE);

        EndDrawing();
    }

    //--------------------
    // Cleanup
    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    ServerStop(server);
    delete &server;
    ShutdownYojimbo();

    return 0;
}