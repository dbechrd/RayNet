#include "../common/common_lib.h"
#include <queue>

typedef std::queue<PlayerInput> PlayerInputQueue;

struct ServerPlayer {
    bool needsClockSync{};
    Entity entity{};  // TODO(dlb): entityIdx into an actual world state
    PlayerInputQueue inputQueue{};
};

typedef World<ServerPlayer> ServerWorld;

ServerWorld *g_world;
uint64_t tick = 0;
double tickAccum = 0;
double lastTickedAt = 0;

class ServerNetAdapter : public NetAdapter
{
public:

    void OnServerClientConnected(int clientIdx)
    {
        ServerPlayer &serverPlayer = g_world->players[clientIdx];
        serverPlayer.needsClockSync = true;

        static const Color colors[]{
            MAROON,
            LIME,
            SKYBLUE
        };
        serverPlayer.entity.color = colors[clientIdx % (sizeof(colors) / sizeof(colors[0]))];
        serverPlayer.entity.size = { 32, 64 };
        serverPlayer.entity.position = { 100, 100 };
        serverPlayer.entity.speed = WINDOW_HEIGHT / 5;
    }

    void OnServerClientDisconnected(int clientIdx)
    {
        ServerPlayer &serverPlayer = g_world->players[clientIdx];
        serverPlayer.entity.color = GRAY;
    }
};

yojimbo::Server &ServerStart()
{
    static ServerNetAdapter adapter{};
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
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_U_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_PLAYER_INPUT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_PLAYER_STATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    server = new yojimbo::Server(yojimbo::GetDefaultAllocator(), privateKey,
        yojimbo::Address("127.0.0.1", SERVER_PORT), config, adapter, GetTime());

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

void ServerProcessMessages(yojimbo::Server &server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.IsClientConnected(clientIdx)) {
            continue;
        }

        for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
            yojimbo::Message *message = server.ReceiveMessage(clientIdx, channelIdx);
            do {
                if (message) {
                    switch (message->GetType())
                    {
                        case MSG_C_PLAYER_INPUT:
                        {
                            Msg_C_PlayerInput *msgPlayerInput = (Msg_C_PlayerInput *)message;
                            g_world->players[clientIdx].inputQueue.push(msgPlayerInput->playerInput);
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
        }
    }
}

void ServerSendClientSnapshots(yojimbo::Server &server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.IsClientConnected(clientIdx)) {
            continue;
        }
        if (!server.CanSendMessage(clientIdx, CHANNEL_U_PLAYER_STATE)) {
            continue;
        }

        // TODO: Send the world state that's relevant to this particular client
        for (int otherClientIdx = 0; otherClientIdx < yojimbo::MaxClients; otherClientIdx++) {
            if (!server.IsClientConnected(otherClientIdx)) {
                // TODO: Notify other player that this player has dc'd somehow (probably not here)
                continue;
            }

            Msg_S_EntityState *message = (Msg_S_EntityState *)server.CreateMessage(clientIdx, MSG_S_ENTITY_STATE);
            if (message)
            {
                ServerPlayer &player = g_world->players[otherClientIdx];
                message->clientIdx = otherClientIdx;
                message->entityState.serverTime = lastTickedAt;
                player.entity.Serialize(message->entityState);
                server.SendMessage(clientIdx, CHANNEL_U_PLAYER_STATE, message);
            }
        }
    }
}

void ServerTick(yojimbo::Server &server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.IsClientConnected(clientIdx)) {
            continue;
        }

        // TODO(dlb): Eating the whole input queue indiscriminately is an awful
        // idea, BUT I'm currently sending input faster than the server tick
        // rate, so it's probably sorta less awful for now. I should actually
        // try to figure out which tick/seq the input is and process them more
        // intelligently...?? Maybe??

        PlayerInput combinedInput{};
        PlayerInputQueue &inputQueue = g_world->players[clientIdx].inputQueue;
        while (!inputQueue.empty()) {
            const PlayerInput &input = inputQueue.front();
            combinedInput.north |= input.north;
            combinedInput.west  |= input.west;
            combinedInput.south |= input.south;
            combinedInput.east  |= input.east ;
            inputQueue.pop();
        }

        ServerPlayer &player = g_world->players[clientIdx];
        Vector2 vDelta{};
        if (combinedInput.north) {
            vDelta.y -= 1.0f;
        }
        if (combinedInput.west) {
            vDelta.x -= 1.0f;
        }
        if (combinedInput.south) {
            vDelta.y += 1.0f;
        }
        if (combinedInput.east) {
            vDelta.x += 1.0f;
        }
        if (vDelta.x && vDelta.y) {
            float invLength = 1.0f / sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y);
            vDelta.x *= invLength;
            vDelta.y *= invLength;
        }

        player.entity.velocity.x = vDelta.x * player.entity.speed;
        player.entity.velocity.y = vDelta.y * player.entity.speed;
        player.entity.position.x += player.entity.velocity.x * SERVER_TICK_DT;
        player.entity.position.y += player.entity.velocity.y * SERVER_TICK_DT;
    }
    tick++;
    lastTickedAt = server.GetTime();
}

void ServerSendClockSync(yojimbo::Server &server)
{
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.IsClientConnected(clientIdx)) {
            continue;
        }

        ServerPlayer &serverPlayer = g_world->players[clientIdx];
        if (serverPlayer.needsClockSync && server.CanSendMessage(clientIdx, CHANNEL_U_CLOCK_SYNC)) {
            Msg_S_ClockSync *msgClockSync = (Msg_S_ClockSync *)server.CreateMessage(clientIdx, MSG_S_CLOCK_SYNC);
            if (msgClockSync)
            {
                msgClockSync->serverTime = GetTime();
                server.SendMessage(clientIdx, CHANNEL_U_CLOCK_SYNC, msgClockSync);
                serverPlayer.needsClockSync = false;
            }
        }
    }
}

void ServerUpdate(yojimbo::Server &server)
{
    if (!server.IsRunning())
        return;

    const double now = GetTime();
    const double dt = now - server.GetTime();
    server.AdvanceTime(now);

    server.ReceivePackets();
    ServerProcessMessages(server);

    tickAccum += dt;
    bool hasDelta = false;
    while (tickAccum >= SERVER_TICK_DT) {
        ServerTick(server);
        tickAccum -= SERVER_TICK_DT;
        hasDelta = true;
    }

    // TODO(dlb): Calculate actual deltas
    if (hasDelta) {
        ServerSendClientSnapshots(server);
    }

    ServerSendClockSync(server);
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
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
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
    g_world = new ServerWorld; //(ServerWorld *)calloc(1, sizeof(*g_world));  // cuz fuck C++
    if (!g_world) {
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
            ServerPlayer &player = g_world->players[clientIdx];
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

            DRAW_TEXT("time", "%.02f", server.GetTime());
            DRAW_TEXT("tick", "%" PRIu64, tick);
            DRAW_TEXT("tickAccum", "%.02f", tickAccum);
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
    delete g_world;

    ServerStop(server);
    delete &server;
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}