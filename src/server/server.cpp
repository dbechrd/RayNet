#include "../common/common_lib.h"

class ServerNetAdapter : public NetAdapter
{
public:

    void OnServerClientConnected(int clientIdx)
    {
        static const Color colors[]{
            MAROON,
            LIME,
            SKYBLUE
        };
        Player &player = g_world->players[clientIdx];
        player.color = colors[clientIdx % (sizeof(colors)/sizeof(colors[0]))];
        player.size = { 32, 64 };
        player.position = { 100, 100 };
        player.speed = 5.0f;
    }

    void OnServerClientDisconnected(int clientIdx)
    {
        Player &player = g_world->players[clientIdx];
        player.color = GRAY;
    }
};

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
    static ServerNetAdapter adapter{};

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", SERVER_PORT), config, adapter, 100);

    // NOTE(dlb): This must be the same size as world->players[] array!
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

void ServerUpdate(yojimbo::Server &server, double tickDt)
{
    if (!server.IsRunning())
        return;

    const double serverNow = server.GetTime();
    server.AdvanceTime(serverNow + tickDt);
    server.ReceivePackets();

    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.IsClientConnected(clientIdx)) {
            continue;
        }

        const int channelIdx = 0;
        yojimbo::Message *message = server.ReceiveMessage(clientIdx, channelIdx);
        do {
            if (message) {
                switch (message->GetType())
                {
                    case MSG_C_PLAYER_INPUT:
                    {
                        Msg_C_PlayerInput *msgPlayerInput = (Msg_C_PlayerInput *)message;

                        Player &player = g_world->players[clientIdx];
                        Vector2 vDelta{};
                        if (msgPlayerInput->playerInput.north) {
                            vDelta.y -= 1.0f;
                        }
                        if (msgPlayerInput->playerInput.west) {
                            vDelta.x -= 1.0f;
                        }
                        if (msgPlayerInput->playerInput.south) {
                            vDelta.y += 1.0f;
                        }
                        if (msgPlayerInput->playerInput.east) {
                            vDelta.x += 1.0f;
                        }
                        if (vDelta.x && vDelta.y) {
                            float invLength = 1.0f / sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y);
                            vDelta.x *= invLength;
                            vDelta.y *= invLength;
                        }
                        player.velocity.x = vDelta.x * player.speed;
                        player.velocity.y = vDelta.y * player.speed;
                        player.position.x += player.velocity.x;
                        player.position.y += player.velocity.y;

                        server.ReleaseMessage(clientIdx, message);
                        break;
                    }
                    default:
                    {
                        printf("foo\n");
                        break;
                    }
                    break;
                }
            }
            message = server.ReceiveMessage(clientIdx, channelIdx);
        } while (message);

        // TODO: Send the world state that's relevant to this particular client
        for (int otherClientIdx = 0; otherClientIdx < yojimbo::MaxClients; otherClientIdx++) {
            if (!server.IsClientConnected(otherClientIdx)) {
                continue;
            }

            Msg_S_PlayerState *message = (Msg_S_PlayerState *)server.CreateMessage(clientIdx, MSG_S_PLAYER_STATE);
            if (message)
            {
                message->clientIdx = otherClientIdx;
                message->player = g_world->players[otherClientIdx];
                server.SendMessage(clientIdx, channelIdx, message);
            }
        }
    }


    server.SendPackets();
}

void ServerStop(yojimbo::Server &server)
{
    server.Stop();
}

int main(int argc, char *argv[])
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

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

    double tickStart = 0;
    double tickAccum = 0;

    while (!WindowShouldClose())
    {
        double now = GetTime();
        double tickDt = now - tickStart;
        tickAccum = yojimbo_min(SERVER_TICK_DT * 5, tickAccum + tickDt);

        while (tickAccum > SERVER_TICK_DT) {
            ServerUpdate(server, SERVER_TICK_DT);
            tickAccum -= SERVER_TICK_DT;
        }

        //--------------------
        // Draw
        BeginDrawing();

        ClearBackground(BROWN);

        Vector2 catPos = {
            WINDOW_WIDTH / 2.0f - texture.width / 2.0f,
            WINDOW_HEIGHT / 2.0f - texture.height / 2.0f
        };
        DrawTexture(texture, (int)catPos.x, (int)catPos.y, WHITE);

        for (int i = 0; i < yojimbo::MaxClients; i++) {
            Player &player = g_world->players[i];
            if (player.color.a) {
                player.Draw();
            }
        }

        Vector2 textPos = {
            WINDOW_WIDTH / 2 - textSize.x / 2,
            catPos.y + texture.height + 4
        };
        DrawTextShadowEx(font, text, textPos, (float)FONT_SIZE, RAYWHITE);

        {
            float hud_x = 8.0f;
            float hud_y = 8.0f;
            char buf[128];
            #define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                Vector2 position{ hud_x, hud_y }; \
                DrawTextShadowEx(font, buf, position, (float)FONT_SIZE, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(font, buf, (float)FONT_SIZE, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += FONT_SIZE; \
            }

            #define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT("time", "%f", server.GetTime());
            DRAW_TEXT("clients", "%d", server.GetNumConnectedClients());

            static bool showClientInfo[yojimbo::MaxClients];
            for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
                if (!server.IsClientConnected(clientIdx)) {
                    continue;
                }

                Rectangle clientRowRect{};
                DRAW_TEXT_MEASURE(&clientRowRect,
                    showClientInfo[clientIdx] ? "[-] client" : "[+] client",
                    "%d", clientIdx
                );
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                    && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, clientRowRect))
                {
                    showClientInfo[clientIdx] = !showClientInfo[clientIdx];
                }
                if (showClientInfo[clientIdx]) {
                    hud_x += 16.0f;
                    yojimbo::NetworkInfo netInfo{};
                    server.GetNetworkInfo(clientIdx, netInfo);
                    DRAW_TEXT("  rtt", "%f", netInfo.RTT);
                    DRAW_TEXT("  %% loss", "%f", netInfo.packetLoss);
                    DRAW_TEXT("  sent (kbps)", "%f", netInfo.sentBandwidth);
                    DRAW_TEXT("  recv (kbps)", "%f", netInfo.receivedBandwidth);
                    DRAW_TEXT("  ack  (kbps)", "%f", netInfo.ackedBandwidth);
                    DRAW_TEXT("  sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
                    DRAW_TEXT("  recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
                    DRAW_TEXT("  ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
                    hud_x -= 16.0f;
                }
            }
        }

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