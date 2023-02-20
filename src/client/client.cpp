#include "../common/common_lib.h"
#include <deque>
#include <time.h>

struct ClientPlayer {
    Entity entity{};
    // TODO: Save past N inputs and send them in every input packet for redundancy
    //PlayerInputQueue inputQueue{};
    std::deque<EntityState> snapshotHistory{};
};

typedef World<ClientPlayer> ClientWorld;

ClientWorld *g_world;

double clientTimeDeltaVsServer = 0;  // how far ahead/behind client clock is

void ClientTryConnect(yojimbo::Client &client)
{
    if (!client.IsDisconnected()) {
        return;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::Address serverAddress("127.0.0.1", SERVER_PORT);

    client.InsecureConnect(privateKey, clientId, serverAddress);
}

yojimbo::Client &ClientStart()
{
    static NetAdapter adapter{};
    static yojimbo::Client *client = nullptr;

    if (client) {
        return *client;
    }

    yojimbo::ClientServerConfig config{};
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_U_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_PLAYER_INPUT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_PLAYER_STATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    client = new yojimbo::Client(yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"), config, adapter, GetTime());

    ClientTryConnect(*client);
#if 0
    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
    return *client;
}

void ClientSendInput(yojimbo::Client &client, PlayerInput &playerInput)
{
    const double now = GetTime();

    static double lastNetTick = 0;
    if (now - lastNetTick > CLIENT_SEND_INPUT_DT) {
        Msg_C_PlayerInput *message = (Msg_C_PlayerInput *)client.CreateMessage(MSG_C_PLAYER_INPUT);
        if (message)
        {
            message->playerInput = playerInput;
            client.SendMessage(CHANNEL_U_PLAYER_INPUT, message);
            playerInput = {};
        }
        lastNetTick = now;
    }
}

void ClientProcessMessages(yojimbo::Client &client)
{
    for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
        yojimbo::Message *message = client.ReceiveMessage(channelIdx);
        do {
            if (message) {
                switch (message->GetType())
                {
                    case MSG_S_CLOCK_SYNC:
                    {
                        Msg_S_ClockSync *msgClockSync = (Msg_S_ClockSync *)message;
                        yojimbo::NetworkInfo netInfo{};
                        client.GetNetworkInfo(netInfo);
                        const double approxServerNow = msgClockSync->serverTime; // + netInfo.RTT / 2;
                        clientTimeDeltaVsServer = GetTime() - approxServerNow;
                        break;
                    }
                    case MSG_S_ENTITY_STATE:
                    {
                        Msg_S_EntityState *msgEntityState = (Msg_S_EntityState *)message;
                        ClientPlayer &player = g_world->players[msgEntityState->clientIdx];
                        if (player.snapshotHistory.size() == CLIENT_SNAPSHOT_COUNT) {
                            player.snapshotHistory.pop_front();
                        }
                        player.snapshotHistory.push_back(msgEntityState->entityState);
                        client.ReleaseMessage(message);
                        break;
                    }
                }
            }
            message = client.ReceiveMessage(channelIdx);
        } while (message);
    }
}

void ClientUpdate(yojimbo::Client &client)
{
    const double now = GetTime();

    // NOTE(dlb): This sends keepalive packets
    client.AdvanceTime(now);

    // TODO(dlb): Is it a good idea / necessary to receive packets every frame,
    // rather than at a fixed rate? It seems like it is... right!?
    client.ReceivePackets();
    if (!client.IsConnected()) {
        return;
    }

    ClientProcessMessages(client);

    // Accmulate input every frame
    static PlayerInput playerInputAccum{};
    playerInputAccum.north |= IsKeyDown(KEY_W);
    playerInputAccum.west  |= IsKeyDown(KEY_A);
    playerInputAccum.south |= IsKeyDown(KEY_S);
    playerInputAccum.east  |= IsKeyDown(KEY_D);

    // Send rolled up input at fixed interval
    static double lastNetTick = 0;
    if (now - lastNetTick > CLIENT_SEND_INPUT_DT) {
        ClientSendInput(client, playerInputAccum);
        lastNetTick = now;
    }

    client.SendPackets();
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
}

int main(int argc, char *argv[])
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    SetWindowPosition(
        monitorWidth / 2, // - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );

    Texture2D texture = LoadTexture("resources/cat.png");

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    const char *text = "Meow MEOW meow MeOw!";
    Vector2 textSize = MeasureTextEx(font, text, (float)FONT_SIZE, 1);

    //--------------------
    // Client
    if (!InitializeYojimbo())
    {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return 1;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    yojimbo::Client &client = ClientStart();

    //--------------------
    // World
    g_world = new ClientWorld;
    if (!g_world) {
        printf("error: failed to allocate world\n");
        return 1;
    }

    double frameStart = GetTime();
    double frameDt = 0;

    // TODO: Input history queue

    while (!WindowShouldClose())
    {
        const double now = GetTime();
        {
            frameDt = now - frameStart;
            frameStart = now;
        }

        //--------------------
        // Update
        if (IsKeyPressed(KEY_V)) {
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
            }
        }

        if (IsKeyPressed(KEY_C)) {
            ClientTryConnect(client);
        }

        //--------------------
        // Networking
        ClientUpdate(client);
        const double serverNow = now - clientTimeDeltaVsServer;

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
            ClientPlayer &player = g_world->players[clientIdx];

            const double renderAt = serverNow - SERVER_TICK_DT * 3;

            // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
            const size_t snapshotCount = player.snapshotHistory.size();
            if (snapshotCount < CLIENT_SNAPSHOT_COUNT) {
                continue;
            }

            int snapshotBIdx = 0;
            while (snapshotBIdx < snapshotCount && player.snapshotHistory[snapshotBIdx].serverTime <= renderAt) {
                snapshotBIdx++;
            }

            const EntityState *snapshotA = 0;
            const EntityState *snapshotB = 0;

            if (snapshotBIdx <= 0) {
                snapshotA = &player.snapshotHistory[0];
                snapshotB = &player.snapshotHistory[0];
            } else if (snapshotBIdx >= snapshotCount) {
                snapshotA = &player.snapshotHistory[snapshotCount - 1];
                snapshotB = &player.snapshotHistory[snapshotCount - 1];
            } else {
                snapshotA = &player.snapshotHistory[(size_t)snapshotBIdx - 1];
                snapshotB = &player.snapshotHistory[snapshotBIdx];
            }

            printf("client %d snapshot %d\n", clientIdx, snapshotBIdx);

            float alpha = 0;
            if (snapshotB != snapshotA) {
                alpha = (renderAt - snapshotA->serverTime) /
                        (snapshotB->serverTime - snapshotA->serverTime);
            }

            player.entity.ApplyStateInterpolated(
                player.snapshotHistory[snapshotCount - 2],
                player.snapshotHistory[snapshotCount - 1],
                alpha
            );
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

            DRAW_TEXT("time", "%.02f", now);
            DRAW_TEXT("serverDelta", "%.02f", clientTimeDeltaVsServer);
            DRAW_TEXT("serverTime", "%.02f", serverNow);

            const char *clientStateStr = "unknown";
            yojimbo::ClientState clientState = client.GetClientState();
            switch (clientState) {
                case yojimbo::CLIENT_STATE_ERROR:        clientStateStr = "CLIENT_STATE_ERROR"; break;
                case yojimbo::CLIENT_STATE_DISCONNECTED: clientStateStr = "CLIENT_STATE_DISCONNECTED"; break;
                case yojimbo::CLIENT_STATE_CONNECTING:   clientStateStr = "CLIENT_STATE_CONNECTING"; break;
                case yojimbo::CLIENT_STATE_CONNECTED:    clientStateStr = "CLIENT_STATE_CONNECTED"; break;
            }

            static bool showNetInfo = false;
            Rectangle netInfoRect{};
            DRAW_TEXT_MEASURE(&netInfoRect,
                showNetInfo ? "[-] state" : "[+] state",
                "%s", clientStateStr
            );
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, netInfoRect))
            {
                showNetInfo = !showNetInfo;
            }
            if (showNetInfo) {
                hud_x += 16.0f;
                yojimbo::NetworkInfo netInfo{};
                client.GetNetworkInfo(netInfo);
                DRAW_TEXT("rtt", "%.02f", netInfo.RTT);
                DRAW_TEXT("%% loss", "%.02f", netInfo.packetLoss);
                DRAW_TEXT("sent (kbps)", "%.02f", netInfo.sentBandwidth);
                DRAW_TEXT("recv (kbps)", "%.02f", netInfo.receivedBandwidth);
                DRAW_TEXT("ack  (kbps)", "%.02f", netInfo.ackedBandwidth);
                DRAW_TEXT("sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
                DRAW_TEXT("recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
                DRAW_TEXT("ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
                hud_x -= 16.0f;
            }
        }

        EndDrawing();
    }

    //--------------------
    // Cleanup
    delete g_world;

    ClientStop(client);
    delete &client;
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}