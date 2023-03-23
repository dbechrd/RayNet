#include "../common/shared_lib.h"

typedef World<ServerPlayer> ServerWorld;

struct Server;

class ServerNetAdapter : public NetAdapter
{
public:
    Server *server;

    ServerNetAdapter(Server *server)
    {
        this->server = server;
    }

    void OnServerClientConnected(int clientIdx) override;
    void OnServerClientDisconnected(int clientIdx) override;
};

struct Server {
    ServerNetAdapter adapter{ this };
    yojimbo::Server *yj_server{};

    uint64_t tick{};
    double tickAccum{};
    double lastTickedAt{};

    ServerWorld *world{};

    void OnClientJoin(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        serverPlayer.needsClockSync = true;

        static const Color colors[]{
            MAROON,
            LIME,
            SKYBLUE
        };
        serverPlayer.entity.color = colors[clientIdx % (sizeof(colors) / sizeof(colors[0]))];
        serverPlayer.entity.size = { 32, 64 };
        serverPlayer.entity.position = { 100, 100 };
        serverPlayer.entity.speed = WINDOW_HEIGHT / 2;
    }

    void OnClientLeave(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        serverPlayer = {};
        //serverPlayer.entity.color = GRAY;
    }
};

void ServerNetAdapter::OnServerClientConnected(int clientIdx)
{
    server->OnClientJoin(clientIdx);
}

void ServerNetAdapter::OnServerClientDisconnected(int clientIdx)
{
    server->OnClientLeave(clientIdx);
}

void ServerStart(Server *server)
{
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);

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
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_U_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_ENTITY_STATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    server->yj_server = new yojimbo::Server(
        yojimbo::GetDefaultAllocator(),
        privateKey,
        yojimbo::Address("127.0.0.1", SV_PORT),
        config,
        server->adapter,
        GetTime()
    );

    // NOTE(dlb): This must be the same size as world->players[] array!
    server->yj_server->Start(yojimbo::MaxClients);
    if (!server->yj_server->IsRunning()) {
        printf("yj: Failed to start server\n");
        return;
    }
#if 0
    printf("yj: started server on port %d (insecure)\n", SV_PORT);
    char addressString[256];
    server->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: server address is %s\n", addressString);
#endif
}

void ServerProcessMessages(Server *server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server->yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
            yojimbo::Message *message = server->yj_server->ReceiveMessage(clientIdx, channelIdx);
            while (message) {
                switch (message->GetType()) {
                    case MSG_C_INPUT_COMMANDS:
                    {
                        Msg_C_InputCommands *msgPlayerInput = (Msg_C_InputCommands *)message;
                        server->world->players[clientIdx].inputQueue = msgPlayerInput->cmdQueue;
                        break;
                    }
                    default:
                    {
                        printf("foo\n");
                        break;
                    }
                }
                server->yj_server->ReleaseMessage(clientIdx, message);
                message = server->yj_server->ReceiveMessage(clientIdx, channelIdx);
            }
        }
    }
}

void ServerSendClientSnapshots(Server *server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server->yj_server->IsClientConnected(clientIdx)) {
            continue;
        }
        if (!server->yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_STATE)) {
            continue;
        }

        // TODO: Send the world state that's relevant to this particular client
        for (int otherClientIdx = 0; otherClientIdx < yojimbo::MaxClients; otherClientIdx++) {
            if (!server->yj_server->IsClientConnected(otherClientIdx)) {
                // TODO: Notify other player that this player has dc'd somehow (probably not here)
                continue;
            }

            Msg_S_EntityState *msg = (Msg_S_EntityState *)server->yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_STATE);
            if (msg) {
                ServerPlayer &player = server->world->players[otherClientIdx];
                msg->clientIdx = otherClientIdx;
                msg->entityState.serverTime = server->lastTickedAt;
                player.entity.Serialize(msg->entityState);
                server->yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_STATE, msg);
            }
        }
    }
}

void PlayerTick(ServerPlayer &player, const InputCmd *input) {
    Vector2 vDelta{};

    if (input) {
        if (input->north) {
            vDelta.y -= 1.0f;
        }
        if (input->west) {
            vDelta.x -= 1.0f;
        }
        if (input->south) {
            vDelta.y += 1.0f;
        }
        if (input->east) {
            vDelta.x += 1.0f;
        }
        if (vDelta.x && vDelta.y) {
            float invLength = 1.0f / sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y);
            vDelta.x *= invLength;
            vDelta.y *= invLength;
        }
        player.lastInputSeq = input->seq;
    }

    player.entity.velocity.x = vDelta.x * player.entity.speed;
    player.entity.velocity.y = vDelta.y * player.entity.speed;
    player.entity.position.x += player.entity.velocity.x * SV_TICK_DT;
    player.entity.position.y += player.entity.velocity.y * SV_TICK_DT;
}

void ServerTick(Server *server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server->yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &player = server->world->players[clientIdx];

        const InputCmd *nextCmd = 0;
        auto &inputQueue = server->world->players[clientIdx].inputQueue;
        for (int i = 0; i < inputQueue.size(); i++) {
            const InputCmd &cmd = inputQueue[i];
            if (cmd.seq > player.lastInputSeq) {
                nextCmd = &cmd;
                break;
            }
        }

        PlayerTick(player, nextCmd);
    }
    server->tick++;
    server->lastTickedAt = server->yj_server->GetTime();
}

void ServerSendClockSync(Server *server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server->yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = server->world->players[clientIdx];
        if (serverPlayer.needsClockSync && server->yj_server->CanSendMessage(clientIdx, CHANNEL_U_CLOCK_SYNC)) {
            Msg_S_ClockSync *msg = (Msg_S_ClockSync *)server->yj_server->CreateMessage(clientIdx, MSG_S_CLOCK_SYNC);
            if (msg) {
                msg->serverTime = GetTime();
                server->yj_server->SendMessage(clientIdx, CHANNEL_U_CLOCK_SYNC, msg);
                serverPlayer.needsClockSync = false;
            }
        }
    }
}

void ServerUpdate(Server *server)
{
    if (!server->yj_server->IsRunning())
        return;

    const double now = GetTime();
    const double dt = now - server->yj_server->GetTime();
    server->yj_server->AdvanceTime(now);

    server->yj_server->ReceivePackets();
    ServerProcessMessages(server);

    server->tickAccum += dt;
    bool hasDelta = false;
    while (server->tickAccum >= SV_TICK_DT) {
        ServerTick(server);
        server->tickAccum -= SV_TICK_DT;
        hasDelta = true;
    }

    // TODO(dlb): Calculate actual deltas
    if (hasDelta) {
        ServerSendClientSnapshots(server);
    }

    ServerSendClockSync(server);
    server->yj_server->SendPackets();
}

void ServerStop(Server *server)
{
    server->yj_server->Stop();

    delete server->world;
    server->world = {};
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Image icon = LoadImage("resources/server.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x, // / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Texture2D texture = LoadTexture("resources/cat.png");

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    const char *text = "Listening...";
    Vector2 textSize = MeasureTextEx(font, text, (float)FONT_SIZE, 1);

    //--------------------
    // Server
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!

    Server *server = new Server;
    ServerStart(server);

    //--------------------
    // World
    server->world = new ServerWorld; //(ServerWorld *)calloc(1, sizeof(*g_world));  // cuz fuck C++
    if (!server->world) {
        printf("error: failed to allocate world\n");
        return 1;
    }

    while (!WindowShouldClose())
    {
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

        for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
            ServerPlayer &player = server->world->players[clientIdx];
            if (player.entity.color.a) {
                player.entity.Draw(font, clientIdx);
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

            DRAW_TEXT("time", "%.02f", server->yj_server->GetTime());
            DRAW_TEXT("tick", "%" PRIu64, server->tick);
            DRAW_TEXT("tickAccum", "%.02f", server->tickAccum);
            DRAW_TEXT("clients", "%d", server->yj_server->GetNumConnectedClients());

            static bool showClientInfo[yojimbo::MaxClients];
            for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
                if (!server->yj_server->IsClientConnected(clientIdx)) {
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
                    server->yj_server->GetNetworkInfo(clientIdx, netInfo);
                    DRAW_TEXT("  rtt", "%f.02", netInfo.RTT);
                    DRAW_TEXT("  %% loss", "%.02f", netInfo.packetLoss);
                    DRAW_TEXT("  sent (kbps)", "%.02f", netInfo.sentBandwidth);
                    DRAW_TEXT("  recv (kbps)", "%.02f", netInfo.receivedBandwidth);
                    DRAW_TEXT("  ack  (kbps)", "%.02f", netInfo.ackedBandwidth);
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
    ServerStop(server);
    delete server->yj_server;
    delete server;
    server = {};
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}