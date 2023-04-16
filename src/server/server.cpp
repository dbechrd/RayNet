#include "../common/shared_lib.h"
#include "server_world.h"

class ServerNetAdapter : public NetAdapter
{
public:
    struct Server *server;

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
        serverPlayer.entityId = clientIdx;  // TODO alloc index, or keep all clients in lower indices??

        static const Color colors[]{
            MAROON,
            LIME,
            SKYBLUE
        };

        Entity &entity = world->entities[serverPlayer.entityId];
        entity.type = Entity_Player;
        entity.color = colors[clientIdx % (sizeof(colors) / sizeof(colors[0]))];
        entity.size = { 32, 64 };
        entity.position = { 100, 100 };
        entity.speed = 200;
    }

    void OnClientLeave(int clientIdx)
    {
        ServerPlayer &serverPlayer = world->players[clientIdx];
        Entity &entity = world->entities[serverPlayer.entityId];
        entity = {};
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
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

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
        if (!server->yj_server->CanSendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT)) {
            continue;
        }

        // TODO: Send only the world state that's relevant to this particular client
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server->world->entities[entityId];
            if (!entity.type) {
                continue;
            }

            Msg_S_EntitySnapshot *msg = (Msg_S_EntitySnapshot *)server->yj_server->CreateMessage(clientIdx, MSG_S_ENTITY_SNAPSHOT);
            if (msg) {
                ServerPlayer &serverPlayer = server->world->players[clientIdx];
                entity.Serialize(entityId, msg->entitySnapshot, server->lastTickedAt, serverPlayer.lastInputSeq);
                server->yj_server->SendMessage(clientIdx, CHANNEL_U_ENTITY_SNAPSHOT, msg);
            }
        }
    }
}

void ServerTick(Server *server)
{
    // Update players
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
                player.lastInputSeq = nextCmd->seq;
                break;
            }
        }

        Entity &entity = server->world->entities[player.entityId];
        entity.Tick(nextCmd, SV_TICK_DT);
    }

    // Update bots
    for (int entityId = yojimbo::MaxClients; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = server->world->entities[entityId];
        if (entity.type != Entity_Bot) {
            continue;
        }

        InputCmd aiCmd{};
        if (entity.velocity.x > 0) {
            if (entity.position.x + entity.size.x / 2 >= WINDOW_WIDTH) {
                entity.velocity.x = 0;
                aiCmd.west = true;
            } else {
                aiCmd.east = true;
            }
        } else if (entity.velocity.x < 0) {
            if (entity.position.x - entity.size.x / 2 <= 0) {
                entity.velocity.x = 0;
                aiCmd.east = true;
            } else {
                aiCmd.west = true;
            }
        } else {
            aiCmd.east = true;
        }

        entity.Tick(&aiCmd, SV_TICK_DT);
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

void ServerUpdate(Server *server, double now)
{
    if (!server->yj_server->IsRunning())
        return;

    server->yj_server->AdvanceTime(now);
    server->yj_server->ReceivePackets();
    ServerProcessMessages(server);

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

    //SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Image icon = LoadImage("resources/server.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    //SetWindowPosition(
    //    monitorWidth / 2 - (int)screenSize.x, // / 2,
    //    monitorHeight / 2 - (int)screenSize.y / 2
    //);

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
    server->world = new ServerWorld;
    if (!server->world) {
        printf("error: failed to allocate world\n");
        return 1;
    }

    //-----------------
    // Bots
    Entity &bot1 = server->world->entities[65];  // todo alloc index (freelist?)
    bot1.type = Entity_Bot;
    bot1.color = SKYBLUE;
    bot1.size = { 32, 32 };
    bot1.position = { 200, 200 };
    bot1.speed = 300;
    //-----------------

    double frameStart = GetTime();
    double frameDt = 0;

    while (!WindowShouldClose())
    {
        const double now = GetTime();
        frameDt = now - frameStart;
        frameStart = now;

        server->tickAccum += frameDt;
        if (server->tickAccum >= SV_TICK_DT) {
            printf("[%.2f][%.2f] ServerUpdate %d\n", server->tickAccum, now, (int)server->tick);
            ServerUpdate(server, now);
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

        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server->world->entities[entityId];
            if (entity.type) {
                entity.Draw(font, entityId);
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
                    DRAW_TEXT("  % loss", "%.02f", netInfo.packetLoss);
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
        yojimbo_sleep(0.001);
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