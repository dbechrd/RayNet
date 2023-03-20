#include "../common/shared_lib.h"
#include <deque>
#include <time.h>

struct Controller {
    int nextSeq {};  // next input command sequence number to use

    InputCmd cmdAccum{};       // accumulate input until we're ready to sample
    double lastAccumSample{};      // time we last sampled accumulator
    double lastCommandMsgSent{};   // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples
};

typedef World<ClientEntity> ClientWorld;

Controller g_controller;
ClientWorld g_world;

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

    yojimbo::Address serverAddress("127.0.0.1", SV_PORT);

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
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_ENTITY_STATE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

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

void ClientSendInput(yojimbo::Client &client, const Controller &controller)
{
    const double now = GetTime();

    static double lastNetTick = 0;
    if (now - lastNetTick > CL_SEND_INPUT_DT) {
        Msg_C_InputCommands *message = (Msg_C_InputCommands *)client.CreateMessage(MSG_C_INPUT_COMMANDS);
        if (message) {
            if (controller.cmdQueue.data[0].north) {
                printf("");
            }
            message->cmdQueue = controller.cmdQueue;
            client.SendMessage(CHANNEL_U_INPUT_COMMANDS, message);
        }
        lastNetTick = now;
    }
}

void ClientProcessMessages(yojimbo::Client &client)
{
    for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
        yojimbo::Message *message = client.ReceiveMessage(channelIdx);
        while (message) {
            switch (message->GetType()) {
                case MSG_S_CLOCK_SYNC:
                {
                    Msg_S_ClockSync *msgClockSync = (Msg_S_ClockSync *)message;
                    yojimbo::NetworkInfo netInfo{};
                    client.GetNetworkInfo(netInfo);
                    const double approxServerNow = msgClockSync->serverTime + netInfo.RTT / 2000;
                    clientTimeDeltaVsServer = GetTime() - approxServerNow;
                    break;
                }
                case MSG_S_ENTITY_STATE:
                {
                    Msg_S_EntityState *msgEntityState = (Msg_S_EntityState *)message;
                    ClientEntity &player = g_world.players[msgEntityState->clientIdx];
                    player.snapshots.push(msgEntityState->entityState);
                    break;
                }
            }
            client.ReleaseMessage(message);
            message = client.ReceiveMessage(channelIdx);
        };
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
    g_controller.cmdAccum.north |= IsKeyDown(KEY_W);
    g_controller.cmdAccum.west  |= IsKeyDown(KEY_A);
    g_controller.cmdAccum.south |= IsKeyDown(KEY_S);
    g_controller.cmdAccum.east  |= IsKeyDown(KEY_D);

    // Sample accumulator once per server tick and push command into command queue
    if (now - g_controller.lastAccumSample > SV_TICK_DT) {
        g_controller.cmdAccum.seq = ++g_controller.nextSeq;
        g_controller.cmdQueue.data[g_controller.cmdQueue.nextIdx++] = g_controller.cmdAccum;
        g_controller.cmdQueue.nextIdx %= CL_SEND_INPUT_COUNT;
        g_controller.cmdAccum = {};
    }

    // Send rolled up input at fixed interval
    static double lastNetTick = 0;
    if (now - lastNetTick > CL_SEND_INPUT_DT) {
        ClientSendInput(client, g_controller);
        lastNetTick = now;
    }

    client.SendPackets();
}

void ClientStop(yojimbo::Client &client)
{
    client.Disconnect();
    g_controller = {};
    g_world = {};
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    //SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows fucking sucks at this
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    Image icon = LoadImage("resources/client.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

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
#if 0
    g_world = new ClientWorld;
    if (!g_world) {
        printf("error: failed to allocate world\n");
        return 1;
    }
#endif

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
        if (IsKeyPressed(KEY_X)) {
            ClientStop(client);
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
            ClientEntity &player = g_world.players[clientIdx];

            // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
            const double renderAt = serverNow - SV_TICK_DT * 3;

            size_t snapshotBIdx = 0;
            while (snapshotBIdx < player.snapshots.size()
                && player.snapshots[snapshotBIdx].serverTime <= renderAt)
            {
                snapshotBIdx++;
            }

            const EntityState *snapshotA = 0;
            const EntityState *snapshotB = 0;

            if (snapshotBIdx <= 0) {
                snapshotA = &player.snapshots.oldest();
                snapshotB = &player.snapshots.oldest();
            } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
                snapshotA = &player.snapshots.newest();
                snapshotB = &player.snapshots.newest();
            } else {
                snapshotA = &player.snapshots[snapshotBIdx - 1];
                snapshotB = &player.snapshots[snapshotBIdx];
            }

            // TODO: Move this to DRAW_TEXT in the tree view if we need it
            //printf("client %d snapshot %d\n", clientIdx, snapshotBIdx);
#if CL_DBG_SNAPSHOT_SHADOWS
            for (int i = 0; i < player.snapshots.size(); i++) {
                player.entity.ApplyStateInterpolated(player.snapshots[i], player.snapshots[i], 0.0);
                player.entity.color = Fade(PINK, 0.5);
                player.entity.Draw(font, i);
            }
#endif
            float alpha = 0;
            if (snapshotB != snapshotA) {
                alpha = (renderAt - snapshotA->serverTime) /
                        (snapshotB->serverTime - snapshotA->serverTime);
            }

            player.entity.ApplyStateInterpolated(*snapshotA, *snapshotB, alpha);
            if (player.entity.color.a) {
                player.entity.Draw(font, clientIdx);
                //for (int i = 0; i < snapshotCount; i++) {
                //    player.entity.Draw(font, i);
                //}
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

            DRAW_TEXT("serverTime", "%.02f", serverNow);
            DRAW_TEXT("serverDelta", "%.02f", clientTimeDeltaVsServer);
            DRAW_TEXT("time", "%.02f", now);
            DRAW_TEXT("vsync", "%s", IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");

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
    //delete g_world;

    ClientStop(client);
    delete &client;
    ShutdownYojimbo();

    UnloadFont(font);
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}