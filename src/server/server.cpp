#include "../common/common.h"
#include "../common/net.h"

yojimbo::Server &ServerStart()
{
    static yojimbo::Server *server = nullptr;
    if (server) {
        return *server;
    }

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

    yojimbo::ClientServerConfig config{};

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", SERVER_PORT), config, netAdapter, 100);

    server->Start(yojimbo::MaxClients);
    if (!server->IsRunning()) {
        printf("yj: Failed to start server\n");
        return *server;
    }
#if 0
    printf("yj: started server on port %d (insecure)\n", SERVER_PORT);
    char addressString[256];
    server->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: server address is %s\n", addressString);
#endif
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
                case MSG_PLAYER_STATE:
                {
                    MsgPlayerState *msgPlayerState = (MsgPlayerState *)message;
                    printf("yj: recv MSG_PLAYER_STATE (pos=%.02f,%.02f)\n",
                        msgPlayerState->player.position.x,
                        msgPlayerState->player.position.y
                    );
                    g_world->player = msgPlayerState->player;
                    server.ReleaseMessage(clientIdx, message);
                }
                break;
            }
        }
        message = server.ReceiveMessage(clientIdx, channelIdx);
    } while (message);

    server.AdvanceTime(server.GetTime() + FIXED_DT);
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
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    yojimbo::Server &server = ServerStart();

    //--------------------
    // World
    g_world = (World *)calloc(1, sizeof(*g_world));
    if (!g_world) {
        printf("error: failed to allocate world\n");
        return 1;
    }
    Player &player = g_world->player;

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

        player.Draw();

        Vector2 textPos = {
            WINDOW_WIDTH / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        DrawTextShadowEx(font, text, textPos, (float)FONT_SIZE, RAYWHITE);

        EndDrawing();
    }

    //--------------------
    // Cleanup
    free(g_world);

    ServerStop(server);
    delete &server;
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}