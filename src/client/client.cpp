#include "../common/shared_lib.h"
#include "client_world.h"
#include <deque>
#include <time.h>

static bool CL_DBG_SNAPSHOT_SHADOWS = true;
static bool CL_DBG_PAUSE_HISTOGRAM = false;

struct HistoLine {
    double fps;
    bool doNetTick;
};
typedef RingBuffer<HistoLine, 64> FpsHistogram;

struct Controller {
    int nextSeq {};  // next input command sequence number to use

    InputCmd cmdAccum{};         // accumulate input until we're ready to sample
    double sampleInputAccum{};   // when this fills up, we are due to sample again
    double lastInputSampleAt{};  // time we last sampled accumulator
    double lastCommandSentAt{};  // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples
};

struct Client {
    NetAdapter adapter;
    yojimbo::Client *yj_client{};
    double clientTimeDeltaVsServer{};  // how far ahead/behind client clock is
    double netTickAccum{};             // when this fills up, a net tick is due
    double lastNetTick{};              // for fixed-step networking updates

    Controller controller;
    ClientWorld *world{};
};

void ClientTryConnect(Client *client, double now)
{
    if (!client->yj_client->IsDisconnected()) {
        return;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    yojimbo::Address serverAddress("127.0.0.1", SV_PORT);

    client->yj_client->InsecureConnect(privateKey, clientId, serverAddress);
    client->world = new ClientWorld;
}

void ClientStart(Client *client, double now)
{
    if (!InitializeYojimbo()) {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);

    yojimbo::ClientServerConfig config{};
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_U_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    client->yj_client = new yojimbo::Client(
        yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"),
        config,
        client->adapter,
        now
    );

    ClientTryConnect(client, now);
#if 0
    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
}

void ClientSendInput(Client *client, const Controller &controller)
{
    Msg_C_InputCommands *message = (Msg_C_InputCommands *)client->yj_client->CreateMessage(MSG_C_INPUT_COMMANDS);
    if (message) {
        message->cmdQueue = controller.cmdQueue;
        client->yj_client->SendMessage(CHANNEL_U_INPUT_COMMANDS, message);
    } else {
        printf("Failed to create INPUT_COMMANDS message.\n");
    }
}

void ClientProcessMessages(Client *client, double now)
{
    for (int channelIdx = 0; channelIdx < CHANNEL_COUNT; channelIdx++) {
        yojimbo::Message *message = client->yj_client->ReceiveMessage(channelIdx);
        while (message) {
            switch (message->GetType()) {
                case MSG_S_CLOCK_SYNC:
                {
                    Msg_S_ClockSync *msgClockSync = (Msg_S_ClockSync *)message;
                    yojimbo::NetworkInfo netInfo{};
                    client->yj_client->GetNetworkInfo(netInfo);
                    const double approxServerNow = msgClockSync->serverTime + netInfo.RTT / 2000;
                    client->clientTimeDeltaVsServer = now - approxServerNow;
                    break;
                }
                case MSG_S_ENTITY_SNAPSHOT:
                {
                    Msg_S_EntitySnapshot *msgEntitySnapshot = (Msg_S_EntitySnapshot *)message;
                    Entity &entity = client->world->entities[msgEntitySnapshot->entitySnapshot.id];
                    entity.type = msgEntitySnapshot->entitySnapshot.type;
                    EntityGhost &ghost = client->world->ghosts[msgEntitySnapshot->entitySnapshot.id];
                    ghost.snapshots.push(msgEntitySnapshot->entitySnapshot);
                    break;
                }
            }
            client->yj_client->ReleaseMessage(message);
            message = client->yj_client->ReceiveMessage(channelIdx);
        };
    }
}

void ClientUpdate(Client *client, double now)
{
    // NOTE(dlb): This sends keepalive packets
    client->yj_client->AdvanceTime(now);

    // TODO(dlb): Is it a good idea / necessary to receive packets every frame,
    // rather than at a fixed rate? It seems like it is... right!?
    client->yj_client->ReceivePackets();
    if (!client->yj_client->IsConnected()) {
        return;
    }

    ClientProcessMessages(client, now);

    // Sample accumulator once per server tick and push command into command queue
    if (client->controller.sampleInputAccum >= CL_SAMPLE_INPUT_DT) {
        client->controller.cmdAccum.seq = ++client->controller.nextSeq;
        client->controller.cmdQueue.push(client->controller.cmdAccum);
        client->controller.cmdAccum = {};
        client->controller.lastInputSampleAt = now;
        client->controller.sampleInputAccum -= CL_SAMPLE_INPUT_DT;
    }

    // Send rolled up input at fixed interval
    if (now - client->controller.lastCommandSentAt >= CL_SEND_INPUT_DT) {
        ClientSendInput(client, client->controller);
        client->controller.lastCommandSentAt = now;
    }

    client->yj_client->SendPackets();
}

void ClientStop(Client *client)
{
    client->yj_client->Disconnect();
    client->clientTimeDeltaVsServer = 0;
    client->netTickAccum = 0;
    client->lastNetTick = 0;

    client->controller = {};
    delete client->world;
    client->world = {};
}

void draw_fps_histogram(FpsHistogram &fpsHistogram, int x, int y)
{
    const float barPadding = 1.0f;
    const float barWidth = 2.0f;
    const float histoHeight = 20.0f;

    float maxValue = 0.0f;
    for (int i = 0; i < fpsHistogram.size(); i++) {
        maxValue = MAX(maxValue, fpsHistogram[i].fps);
    }

    const float barScale = histoHeight / maxValue;

    Vector2 cursor{};
    cursor.x = x;
    cursor.y = y;

    for (int i = 0; i < fpsHistogram.size(); i++) {
        float bottom = cursor.y + histoHeight;
        float height = fpsHistogram[i].fps * barScale;

        Rectangle rect{};
        rect.x = cursor.x;
        rect.y = bottom - height;
        rect.width = barWidth;
        rect.height = height;
        DrawRectangleRec(rect, fpsHistogram[i].doNetTick ? GREEN : RAYWHITE);

        cursor.x += barWidth + barPadding;
    }
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    //SetTraceLogLevel(LOG_WARNING);

    const double startedAt = GetTime();
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)startedAt);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows fucking sucks at this
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

    Font font = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    //--------------------
    // Client
    Client *client = new Client;
    ClientStart(client, startedAt);

    FpsHistogram fpsHistogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    while (!WindowShouldClose())
    {
        const double now = GetTime();

        frameDt = MIN(now - frameStart, SV_TICK_DT);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = now;

        client->controller.sampleInputAccum += frameDt;
        client->netTickAccum += frameDt;
        bool doNetTick = client->netTickAccum >= SV_TICK_DT;

        if (!CL_DBG_PAUSE_HISTOGRAM) {
            HistoLine histoLine{ frameDtSmooth, doNetTick };
            fpsHistogram.push(histoLine);
        }

        //--------------------
        // Accmulate input every frame
        client->controller.cmdAccum.north |= IsKeyDown(KEY_W);
        client->controller.cmdAccum.west  |= IsKeyDown(KEY_A);
        client->controller.cmdAccum.south |= IsKeyDown(KEY_S);
        client->controller.cmdAccum.east  |= IsKeyDown(KEY_D);

        //--------------------
        // Update
        if (IsKeyPressed(KEY_V)) {
            bool isFullScreen = IsWindowState(FLAG_FULLSCREEN_MODE);
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
                if (isFullScreen) {
                    SetWindowState(FLAG_FULLSCREEN_MODE);
                }
            }
        }

        if (IsKeyPressed(KEY_C)) {
            ClientTryConnect(client, now);
        }
        if (IsKeyPressed(KEY_X)) {
            ClientStop(client);
        }
        if (IsKeyPressed(KEY_Z)) {
            CL_DBG_SNAPSHOT_SHADOWS = !CL_DBG_SNAPSHOT_SHADOWS;
        }
        if (IsKeyPressed(KEY_H)) {
            CL_DBG_PAUSE_HISTOGRAM = !CL_DBG_PAUSE_HISTOGRAM;
        }

        //--------------------
        // Networking
        if (doNetTick) {
            ClientUpdate(client, now);
            client->lastNetTick = now;
            client->netTickAccum -= SV_TICK_DT;
        }
        const double serverNow = now - client->clientTimeDeltaVsServer;

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(CLITERAL(Color){ 20, 60, 30, 255 });

        if (client->yj_client->IsConnected()) {
            for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
                Entity &entity = client->world->entities[entityId];
                if (entity.type == Entity_None) {
                    continue;
                }

                EntityGhost &ghost = client->world->ghosts[entityId];

                // TODO(dlb): Find snapshots nearest to (GetTime() - clientTimeDeltaVsServer)
                const double renderAt = serverNow - SV_TICK_DT;

                size_t snapshotBIdx = 0;
                while (snapshotBIdx < ghost.snapshots.size()
                    && ghost.snapshots[snapshotBIdx].serverTime <= renderAt)
                {
                    snapshotBIdx++;
                }

                const EntitySnapshot *snapshotA = 0;
                const EntitySnapshot *snapshotB = 0;

                if (snapshotBIdx <= 0) {
                    snapshotA = &ghost.snapshots.oldest();
                    snapshotB = &ghost.snapshots.oldest();
                } else if (snapshotBIdx >= CL_SNAPSHOT_COUNT) {
                    snapshotA = &ghost.snapshots.newest();
                    snapshotB = &ghost.snapshots.newest();
                } else {
                    snapshotA = &ghost.snapshots[snapshotBIdx - 1];
                    snapshotB = &ghost.snapshots[snapshotBIdx];
                }

                // TODO: Move this to DRAW_TEXT in the tree view if we need it
                //printf("client %d snapshot %d\n", clientIdx, snapshotBIdx);
                if (CL_DBG_SNAPSHOT_SHADOWS) {
                    for (int i = 0; i < ghost.snapshots.size(); i++) {
                        Entity &entity = client->world->entities[entityId];
                        entity.ApplyStateInterpolated(ghost.snapshots[i], ghost.snapshots[i], 0.0);
                        entity.color = Fade(GRAY, 0.5);
                        entity.Draw(font, i);
                    }
                }

                float alpha = 0;
                if (snapshotB != snapshotA) {
                    alpha = (renderAt - snapshotA->serverTime) /
                            (snapshotB->serverTime - snapshotA->serverTime);
                }

                if (entityId == client->yj_client->GetClientIndex()) {
                    uint32_t lastProcessedInputCmd = 0;
                    if (ghost.snapshots.size()) {
                        const EntitySnapshot &latestSnapshot = ghost.snapshots.newest();
                        entity.ApplyStateInterpolated(ghost.snapshots.newest(), ghost.snapshots.newest(), 0);
                        lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
                    }

                    Color entityColor = entity.color;

                    const double cmdAccumDt = now - client->controller.lastInputSampleAt;
                    //printf(" accumDt=%5.2f", cmdAccumDt);
                    //printf("  %u |", lastProcessedInputCmd);
                    for (size_t cmdIndex = 0; cmdIndex < client->controller.cmdQueue.size(); cmdIndex++) {
                        InputCmd &inputCmd = client->controller.cmdQueue[cmdIndex];
                        if (inputCmd.seq > lastProcessedInputCmd) {
                            //printf(" %d", inputCmd.seq);
                            entity.Tick(&inputCmd, SV_TICK_DT);
                            if (CL_DBG_SNAPSHOT_SHADOWS) {
                                entity.color = Fade(SKYBLUE, 0.5);
                                entity.Draw(font, inputCmd.seq);
                            }
                        }
                    }
                    //entity.Tick(&client->controller.cmdAccum, cmdAccumDt);
                    entity.color = entityColor;

                    uint32_t oldestInput = client->controller.cmdQueue.oldest().seq;
                    if (oldestInput > lastProcessedInputCmd + 1) {
                        //printf(" (%d inputs dropped)", oldestInput - lastProcessedInputCmd - 1);
                    }

                    //putchar('\n');
                } else {
                    entity.ApplyStateInterpolated(*snapshotA, *snapshotB, alpha);
                }

                if (entity.color.a) {
                    entity.Draw(font, entityId);
                }
            }
        }

        {
            float hud_x = 8.0f;
            float hud_y = 30.0f;
            char buf[128];
            #define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-11s : " fmt, label, __VA_ARGS__); \
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

            DRAW_TEXT("serverTime", "%.2f (%s%.2f)", serverNow, client->clientTimeDeltaVsServer > 0 ? "+" : "", client->clientTimeDeltaVsServer);
            DRAW_TEXT("time", "%.2f", now);
            DRAW_TEXT("vsync", "%s", IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("frameDt", "%.2f fps (%.2f ms)", 1.0 / frameDt, frameDt);

            const char *clientStateStr = "unknown";
            yojimbo::ClientState clientState = client->yj_client->GetClientState();
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
                client->yj_client->GetNetworkInfo(netInfo);
                DRAW_TEXT("rtt", "%.2f", netInfo.RTT);
                DRAW_TEXT("% loss", "%.2f", netInfo.packetLoss);
                DRAW_TEXT("sent (kbps)", "%.2f", netInfo.sentBandwidth);
                DRAW_TEXT("recv (kbps)", "%.2f", netInfo.receivedBandwidth);
                DRAW_TEXT("ack  (kbps)", "%.2f", netInfo.ackedBandwidth);
                DRAW_TEXT("sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
                DRAW_TEXT("recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
                DRAW_TEXT("ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
                hud_x -= 16.0f;
            }
        }

        draw_fps_histogram(fpsHistogram, 8, 8);

        EndDrawing();
        //yojimbo_sleep(0.001);
    }

    //--------------------
    // Cleanup
    ClientStop(client);
    delete client->yj_client;
    client->yj_client = {};
    delete client;
    ShutdownYojimbo();

    UnloadFont(font);
    CloseWindow();

    return 0;
}