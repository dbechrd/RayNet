#include "../common/shared_lib.h"
#include "../common/histogram.h"
#include "client_world.h"
#include "game_client.h"
#include <deque>
#include <time.h>

static bool CL_DBG_SNAPSHOT_SHADOWS = true;
Font fntHackBold20;

int ClientTryConnect(GameClient *client)
{
    if (!client->yj_client->IsDisconnected()) {
        printf("yj: client already connected, disconnect first\n");
        return -1;
    }

    uint8_t privateKey[yojimbo::KeyBytes];
    memset(privateKey, 0, yojimbo::KeyBytes);

    uint64_t clientId = 0;
    yojimbo::random_bytes((uint8_t *)&clientId, 8);
    printf("yj: client id is %.16" PRIx64 "\n", clientId);

    //yojimbo::Address serverAddress("127.0.0.1", SV_PORT);
    //yojimbo::Address serverAddress("192.168.0.143", SV_PORT);
    yojimbo::Address serverAddress("68.9.219.64", SV_PORT);
    //yojimbo::Address serverAddress("slime.theprogrammingjunkie.com", SV_PORT);

    if (!serverAddress.IsValid()) {
        printf("yj: invalid address\n");
        return -1;
    }

    client->yj_client->InsecureConnect(privateKey, clientId, serverAddress);
    client->world = new ClientWorld;
    client->world->camera2d.zoom = 1.0f;
    client->world->map.Load(LEVEL_001);
    return 0;
}

void ClientStart(GameClient *client)
{
    if (!InitializeYojimbo()) {
        printf("yj: error: failed to initialize Yojimbo!\n");
        return;
    }
    yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
    yojimbo_set_printf_function(yj_printf);

    yojimbo::ClientServerConfig config{};
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_R_CLOCK_SYNC     ].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_INPUT_COMMANDS ].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_ENTITY_EVENT   ].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    client->yj_client = new yojimbo::Client(
        yojimbo::GetDefaultAllocator(),
        yojimbo::Address("0.0.0.0"),
        config,
        client->adapter,
        client->now
    );

    if (ClientTryConnect(client) < 0) {
        printf("Failed to connect to server\n");
    }
#if 0
    char addressString[256];
    client->GetAddress().ToString(addressString, sizeof(addressString));
    printf("yj: client address is %s\n", addressString);
#endif
}

void ClientSendInput(GameClient *client, const Controller &controller)
{
    if (client->yj_client->CanSendMessage(MSG_C_INPUT_COMMANDS)) {
        Msg_C_InputCommands *message = (Msg_C_InputCommands *)client->yj_client->CreateMessage(MSG_C_INPUT_COMMANDS);
        if (message) {
            message->cmdQueue = controller.cmdQueue;
            client->yj_client->SendMessage(CHANNEL_U_INPUT_COMMANDS, message);
        } else {
            printf("Failed to create INPUT_COMMANDS message.\n");
        }
    } else {
        printf("Outgoing INPUT_COMMANDS channel message queue is full.\n");
    }
}

void ClientSendEntityInteract(GameClient *client, uint32_t entityId)
{
    if (client->yj_client->CanSendMessage(MSG_C_ENTITY_INTERACT)) {
        Msg_C_EntityInteract *message = (Msg_C_EntityInteract *)client->yj_client->CreateMessage(MSG_C_ENTITY_INTERACT);
        if (message) {
            message->entityId = entityId;
            client->yj_client->SendMessage(MSG_C_ENTITY_INTERACT, message);
        } else {
            printf("Failed to create ENTITY_INTERACT message.\n");
        }
    } else {
        printf("Outgoing ENTITY_INTERACT channel message queue is full.\n");
    }
}

void ClientProcessMessages(GameClient *client)
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
                    client->clientTimeDeltaVsServer = client->now - approxServerNow;
                    break;
                }
                case MSG_S_ENTITY_DESPAWN:
                {
                    Msg_S_EntityDespawn *msgEntityDespawn = (Msg_S_EntityDespawn *)message;
                    Entity &entity = client->world->entities[msgEntityDespawn->entityId];
                    entity = {};
                    EntityGhost &ghost = client->world->ghosts[msgEntityDespawn->entityId];
                    ghost.snapshots = {};
                    break;
                }
                case MSG_S_ENTITY_SAY:
                {
                    Msg_S_EntitySay *msgEntitySay = (Msg_S_EntitySay *)message;
                    Entity &entity = client->world->entities[msgEntitySay->entityId];
                    if (msgEntitySay->messageLength > 0 && msgEntitySay->messageLength <= SV_MAX_ENTITY_SAY_MSG_LEN) {
                        client->world->CreateDialog(
                            msgEntitySay->entityId,
                            msgEntitySay->messageLength,
                            msgEntitySay->message,
                            client->now
                        );
                    } else {
                        printf("Wtf dis server smokin'? Sent entity say message of length %u but max is %u\n",
                            msgEntitySay->messageLength,
                            SV_MAX_ENTITY_SAY_MSG_LEN);
                    }
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
                case MSG_S_ENTITY_SPAWN:
                {
                    Msg_S_EntitySpawn *msgEntitySpawn = (Msg_S_EntitySpawn *)message;
                    Entity &entity = client->world->entities[msgEntitySpawn->entitySpawnEvent.id];
                    entity.ApplySpawnEvent(msgEntitySpawn->entitySpawnEvent);
                    break;
                }
            }
            client->yj_client->ReleaseMessage(message);
            message = client->yj_client->ReceiveMessage(channelIdx);
        };
    }
}

void ClientUpdate(GameClient *client)
{
    // NOTE(dlb): This sends keepalive packets
    client->yj_client->AdvanceTime(client->now);

    // TODO(dlb): Is it a good idea / necessary to receive packets every frame,
    // rather than at a fixed rate? It seems like it is... right!?
    client->yj_client->ReceivePackets();
    if (!client->yj_client->IsConnected()) {
        return;
    }

    ClientProcessMessages(client);

    // Sample accumulator once per server tick and push command into command queue
    if (client->controller.sampleInputAccum >= CL_SAMPLE_INPUT_DT) {
        client->controller.cmdAccum.seq = ++client->controller.nextSeq;
        client->controller.cmdQueue.push(client->controller.cmdAccum);
        client->controller.cmdAccum = {};
        client->controller.lastInputSampleAt = client->now;
        client->controller.sampleInputAccum -= CL_SAMPLE_INPUT_DT;
    }

    // Send rolled up input at fixed interval
    if (client->now - client->controller.lastCommandSentAt >= CL_SEND_INPUT_DT) {
        ClientSendInput(client, client->controller);
        client->controller.lastCommandSentAt = client->now;
    }

    client->yj_client->SendPackets();
}

void ClientStop(GameClient *client)
{
    client->yj_client->Disconnect();
    client->clientTimeDeltaVsServer = 0;
    client->netTickAccum = 0;
    client->lastNetTick = 0;
    client->now = 0;

    client->controller = {};
    delete client->world;
    client->world = {};
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
    //SetWindowState(FLAG_WINDOW_RESIZABLE);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    Image icon = LoadImage("resources/client.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

#if CL_DBG_ONE_SCREEN
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    SetWindowPosition(
        monitorWidth / 2, // - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );
#elif CL_DBG_TWO_SCREEN
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);
    SetWindowPosition(
        monitorWidth / 2 - (int)screenSize.x / 2,
        monitorHeight / 2 - (int)screenSize.y / 2
    );
#endif

    fntHackBold20 = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);

    //--------------------
    // Client
    GameClient *client = new GameClient;
    client->now = GetTime();
    ClientStart(client);

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    while (!WindowShouldClose())
    {
        client->now = GetTime();
        frameDt = MIN(client->now - frameStart, SV_TICK_DT);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = client->now;

        client->controller.sampleInputAccum += frameDt;
        client->netTickAccum += frameDt;
        bool doNetTick = client->netTickAccum >= SV_TICK_DT;

        //--------------------
        // Input
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
            ClientTryConnect(client);
        }
        if (IsKeyPressed(KEY_X)) {
            ClientStop(client);
        }
        if (IsKeyPressed(KEY_Z)) {
            CL_DBG_SNAPSHOT_SHADOWS = !CL_DBG_SNAPSHOT_SHADOWS;
        }
        if (IsKeyPressed(KEY_H)) {
            histogram.paused = !histogram.paused;
        }

        Vector2 cursorWorldPos{};

        //--------------------
        // Accmulate input every frame
        if (client->yj_client->IsConnected()) {
            Entity &localPlayer = client->world->entities[client->LocalPlayerEntityId()];

            client->controller.cmdAccum.north |= IsKeyDown(KEY_W);
            client->controller.cmdAccum.west |= IsKeyDown(KEY_A);
            client->controller.cmdAccum.south |= IsKeyDown(KEY_S);
            client->controller.cmdAccum.east |= IsKeyDown(KEY_D);
            client->controller.cmdAccum.fire |= IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), client->world->camera2d);
            Vector2 facing = Vector2Subtract(cursorWorldPos, localPlayer.position);
            facing = Vector2Normalize(facing);
            client->controller.cmdAccum.facing = facing;
        }

        //--------------------
        // Networking
        if (doNetTick) {
            ClientUpdate(client);
            client->lastNetTick = client->now;
            client->netTickAccum -= SV_TICK_DT;
        }

        //--------------------
        // Update
        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

        if (client->yj_client->IsConnected()) {
            client->world->Update(*client);
        }

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(CLITERAL(Color){ 20, 60, 30, 255 });

        if (client->yj_client->IsConnected()) {
            //--------------------
            // Camera
            // TODO: Move update code out of draw code and update local player's
            // position before using it to determine camera location... doh!
            {
                Entity &localPlayer = client->world->entities[client->LocalPlayerEntityId()];
                client->world->camera2d.offset = { (float)GetScreenWidth()/2, (float)GetScreenHeight()/2 };
                client->world->camera2d.target = { localPlayer.position.x, localPlayer.position.y };
            }

            //--------------------
            // Draw the map
            BeginMode2D(client->world->camera2d);
            client->world->map.Draw(client->world->camera2d);

            //--------------------
            // Draw the entities
            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), client->world->camera2d);
            for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
                Entity &entity = client->world->entities[entityId];
                if (entity.type == Entity_None) {
                    continue;
                }
                EntityGhost &ghost = client->world->ghosts[entityId];

                if (CL_DBG_SNAPSHOT_SHADOWS) {
                    Entity ghostInstance = client->world->entities[entityId];
                    for (int i = 0; i < ghost.snapshots.size(); i++) {
                        if (!ghost.snapshots[i].serverTime) {
                            continue;
                        }
                        ghostInstance.ApplyStateInterpolated(ghost.snapshots[i], ghost.snapshots[i], 0.0);
                        ghostInstance.color = Fade(GRAY, 0.5);

                        const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
                        ghostInstance.Draw(fntHackBold20, i, scalePer + i * scalePer);
                    }

#if CL_CLIENT_SIDE_PREDICT
                    // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
                    if (entityId == client->LocalPlayerEntityId()) {
                        Entity predictionInstance = client->world->entities[entityId];

                        uint32_t lastProcessedInputCmd = 0;
                        if (ghost.snapshots.size()) {
                            const EntitySnapshot &latestSnapshot = ghost.snapshots.newest();
                            predictionInstance.ApplyStateInterpolated(ghost.snapshots.newest(), ghost.snapshots.newest(), 0);
                            lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
                        }

                        //predictionInstance.color = Fade(SKYBLUE, 0.5);
                        predictionInstance.color = Fade(DARKGRAY, 0.5);

                        //const double cmdAccumDt = client->now - client->controller.lastInputSampleAt;
                        for (size_t cmdIndex = 0; cmdIndex < client->controller.cmdQueue.size(); cmdIndex++) {
                            InputCmd &inputCmd = client->controller.cmdQueue[cmdIndex];
                            if (inputCmd.seq > lastProcessedInputCmd) {
                                predictionInstance.ApplyForce(inputCmd.GenerateMoveForce(predictionInstance.speed));
                                predictionInstance.Tick(client->now, SV_TICK_DT);
                                client->world->map.ResolveEntityTerrainCollisions(predictionInstance);
                                predictionInstance.Draw(fntHackBold20, inputCmd.seq, 1);
                            }
                        }
                        //predictionInstance.Tick(&client->controller.cmdAccum, cmdAccumDt);
                    }
#endif
                }

                const Color origColor = entity.color;
                if (dlb_CheckCollisionPointRec(cursorWorldPos, entity.GetRect())) {
                    entity.color = ColorBrightness(entity.color, 0.2f);
                    bool down = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                    if (down) {
                        ClientSendEntityInteract(client, entityId);
                    }
                }
                entity.Draw(fntHackBold20, entityId, 1);
                entity.color = origColor;
            }

            //--------------------
            // Draw the dialogs
            client->world->Draw();

            EndMode2D();
        }

        {
            float hud_x = 8.0f;
            float hud_y = 30.0f;
            char buf[128];
            #define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-11s : " fmt, label, __VA_ARGS__); \
                Vector2 position{ hud_x, hud_y }; \
                DrawTextShadowEx(fntHackBold20, buf, position, (float)fntHackBold20.baseSize, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntHackBold20, buf, (float)fntHackBold20.baseSize, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += fntHackBold20.baseSize; \
            }

            #define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT("frameDt", "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("serverTime", "%.2f (%s%.2f)", client->ServerNow(), client->clientTimeDeltaVsServer > 0 ? "+" : "", client->clientTimeDeltaVsServer);
            DRAW_TEXT("localTime", "%.2f", client->now);
            DRAW_TEXT("cursor", "%d, %d", GetMouseX(), GetMouseY());
            DRAW_TEXT("cursor", "%d, %d (world: %.f, %.f)", GetMouseX(), GetMouseY(), cursorWorldPos.x, cursorWorldPos.y);

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

        histogram.Draw(8, 8);
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

    UnloadFont(fntHackBold20);
    CloseWindow();

    return 0;
}