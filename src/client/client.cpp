#include "../common/shared_lib.h"
#include "../common/histogram.h"
#include "../common/ui/ui.h"
#include "client_world.h"
#include "game_client.h"
#include "todo.h"

static bool CL_DBG_SNAPSHOT_SHADOWS = false;

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    Err err = RN_SUCCESS;

    //SetTraceLogLevel(LOG_WARNING);

    const double startedAt = GetTime();
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)startedAt);

    InitAudioDevice();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Client");
    //SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows fucking sucks at this
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

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
    }

    //--------------------
    // Client
    GameClient *client = new GameClient;
    client->now = GetTime();
    client->Start();

    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    bool showNetInfo = false;
    bool showTodoList = false;
    static float texMenuBgScale = 0;

    SetExitKey(0);
    bool quit = false;

    while (!quit) {
        client->now = GetTime();
        frameDt = MIN(client->now - frameStart, SV_TICK_DT);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = client->now;

        client->controller.sampleInputAccum += frameDt;
        client->netTickAccum += frameDt;
        bool doNetTick = client->netTickAccum >= SV_TICK_DT;

        bool escape = IsKeyPressed(KEY_ESCAPE);

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
            client->TryConnect();
        }
        if (IsKeyPressed(KEY_X)) {
            client->Stop();
        }
        if (IsKeyPressed(KEY_Z)) {
            CL_DBG_SNAPSHOT_SHADOWS = !CL_DBG_SNAPSHOT_SHADOWS;
        }
        if (IsKeyPressed(KEY_T)) {
            showTodoList = !showTodoList;
            if (showTodoList) {
                client->todoList.Load(TODO_LIST_PATH);
            }
        }

        Vector2 cursorWorldPos{};

        //--------------------
        // Accmulate input every frame
        Entity *localPlayer = client->LocalPlayer();
        if (localPlayer) {
            client->controller.cmdAccum.north |= IsKeyDown(KEY_W);
            client->controller.cmdAccum.west |= IsKeyDown(KEY_A);
            client->controller.cmdAccum.south |= IsKeyDown(KEY_S);
            client->controller.cmdAccum.east |= IsKeyDown(KEY_D);
            client->controller.cmdAccum.fire |= IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), client->world->camera2d);
            Vector2 facing = Vector2Subtract(cursorWorldPos, localPlayer->position);
            facing = Vector2Normalize(facing);
            client->controller.cmdAccum.facing = facing;
        }

        if (client->yj_client->IsConnected()) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Tilemap::Coord coord{};
                if (client->world->map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord)) {
                    client->SendTileInteract(coord.x, coord.y);
                }
            }
        }

        //--------------------
        // Networking
        if (doNetTick) {
            texMenuBgScale += 0.001f;
            if (texMenuBgScale > 1.1f) {
                texMenuBgScale = 0;
            }

            client->Update();
            client->lastNetTick = client->now;
            client->netTickAccum -= SV_TICK_DT;
        }

        //--------------------
        // Update
        if (client->yj_client->IsConnected()) {
            client->world->Update(*client);
        }

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(GRAYISH_BLUE);

        uint32_t hoveredEntityId = 0;

        localPlayer = client->LocalPlayer();
        if (localPlayer) {
            if (client->LocalPlayerEntityId() != 1) {
                SetMasterVolume(0);
            }

            //--------------------
            // Camera
            // TODO: Move update code out of draw code and update local player's
            // position before using it to determine camera location... doh!
            client->world->camera2d.offset = { (float)GetScreenWidth()/2, (float)GetScreenHeight()/2 };
            client->world->camera2d.target = { localPlayer->position.x, localPlayer->position.y };

            //--------------------
            // Draw the map
            BeginMode2D(client->world->camera2d);
            client->world->map.Draw(client->world->camera2d, false);

            //--------------------
            // Draw the entities
            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), client->world->camera2d);
            for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
                Entity *entity = client->world->GetEntity(entityId);
                if (!entity) {
                    continue;
                }
                EntityGhost &ghost = client->world->ghosts[entityId];

                if (CL_DBG_SNAPSHOT_SHADOWS) {
                    Entity ghostInstance = *entity;
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
                        Entity predictionInstance = *entity;

                        uint32_t lastProcessedInputCmd = 0;
                        const EntitySnapshot &latestSnapshot = ghost.snapshots.newest();
                        if (latestSnapshot.serverTime) {
                            predictionInstance.ApplyStateInterpolated(latestSnapshot, latestSnapshot, 0);
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

                const Color origColor = entity->color;
                bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, entity->GetRect());
                if (hover) {
                    entity->color = ColorBrightness(entity->color, 0.2f);
                    bool down = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                    if (down) {
                        client->SendEntityInteract(entityId);
                    }
                    hoveredEntityId = entityId;
                }
                entity->Draw(fntHackBold20, entityId, 1);
                entity->color = origColor;
            }

            //--------------------
            // Draw the dialogs
            client->world->Draw();

            EndMode2D();
        } else {
            SetMasterVolume(1);

            Vector2 uiPosition{ screenSize.x / 2, screenSize.y / 2 - 50 };
            UIStyle uiStyleMenu {};
            uiStyleMenu.pad = { 16, 4 };
            uiStyleMenu.bgColor = BLANK;
            uiStyleMenu.fgColor = RAYWHITE;
            uiStyleMenu.font = &fntHackBold32;
            uiStyleMenu.alignH = TextAlign_Center;
            UI uiMenu{ uiPosition, uiStyleMenu };

            BeginShaderMode(shdSdfText);    // Activate SDF font shader

            const char *connectingStrs[] = {
                "Connecting   ",
                "Connecting.  ",
                "Connecting.. ",
                "Connecting..."
            };
            static int connectingDotIdx = 0;
            static double connectingDotIdxLastUpdatedAt = 0;
            if (client->yj_client->IsConnecting()) {
                if (!connectingDotIdxLastUpdatedAt) {
                    connectingDotIdxLastUpdatedAt = client->now;
                } else if (client->now > connectingDotIdxLastUpdatedAt + 0.5) {
                    connectingDotIdxLastUpdatedAt = client->now;
                    connectingDotIdx = ((size_t)connectingDotIdx + 1) % ARRAY_SIZE(connectingStrs);
                }
            } else {
                connectingDotIdx = 0;
                connectingDotIdxLastUpdatedAt = 0;
            }

            if (client->yj_client->IsConnected()) {
                uiMenu.Text("Loading...");
            } else if (client->yj_client->IsConnecting()) {
                uiMenu.Text(connectingStrs[connectingDotIdx]);
            } else {
                const Vector2 screenHalfSize{ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
                const Vector2 screenCenter{ screenHalfSize.x, screenHalfSize.y };
                for (float scale = 0.0f; scale < 1.1f; scale += 0.1f) {
                    const float modScale = fmodf(texMenuBgScale + scale, 1);
                    Rectangle menuBgRect{
                        screenCenter.x - screenHalfSize.x * modScale,
                        screenCenter.y - screenHalfSize.y * modScale,
                        screenSize.x * modScale,
                        screenSize.y * modScale
                    };
                    //DrawRectangleLinesEx(menuBgRect, 20, BLACK);
                }

                UIState connectButton = uiMenu.Button("Play");
                if (connectButton.clicked) {
                    //rnSoundSystem.Play(RN_Sound_Lily_Introduction);
                    client->TryConnect();
                }
                uiMenu.Newline();
                uiMenu.Button("Options");
                uiMenu.Newline();
                UIState quitButton = uiMenu.Button("Quit");
                if (quitButton.clicked) {
                    quit = true;
                }

                // Draw font atlas for SDF font
                //DrawTexture(fntHackBold32.texture, GetScreenWidth() - fntHackBold32.texture.width, 0, WHITE);
            }

            EndShaderMode();
        }

        // HP bar
        if (hoveredEntityId) {
            Entity *entity = client->world->GetEntity(hoveredEntityId);
            assert(entity); // huh?
            if (entity) {
                entity->DrawHoverInfo();
            }
        }

        // TODO: Quit button
        UIStyle uiHUDMenuStyle{};
        UI uiHUDMenu{ {}, uiHUDMenuStyle };

        uiHUDMenu.Button("Menu");
        uiHUDMenu.Button("Quests");
        uiHUDMenu.Button("Inventory");
        uiHUDMenu.Button("Map");
        uiHUDMenu.Newline();

        Vector2 hackNewCursor{ uiHUDMenu.CursorScreen() };

        // TODO: ui.histogram(histogram);
        {
            static Histogram fpsHistogram{};

            if (IsKeyPressed(KEY_H)) {
                // TODO: fpsHistogram.update() which checks input and calls TogglePause?
                fpsHistogram.paused = !fpsHistogram.paused;
            }
            fpsHistogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

            hackNewCursor.x += 8;
            hackNewCursor.y += 8;
            fpsHistogram.Draw(hackNewCursor);
            hackNewCursor.y += fpsHistogram.histoHeight + 8;
        }

        Vector2 uiPosition{ hackNewCursor };

        // Debug HUD
        {
            char buf[128];
            #define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-11s : " fmt, label, __VA_ARGS__); \
                DrawTextShadowEx(fntHackBold20, buf, uiPosition, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntHackBold20, buf, (float)fntHackBold20.baseSize, 1.0); \
                    *measureRect = { uiPosition.x, uiPosition.y, measure.x, measure.y }; \
                } \
                uiPosition.y += fntHackBold20.baseSize; \
            }

            #define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT("frameDt", "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("serverTime", "%.2f (%s%.2f)", client->ServerNow(), client->clientTimeDeltaVsServer > 0 ? "+" : "", client->clientTimeDeltaVsServer);
            DRAW_TEXT("localTime", "%.2f", client->now);
            DRAW_TEXT("cursor", "%d, %d (world: %.f, %.f)", GetMouseX(), GetMouseY(), cursorWorldPos.x, cursorWorldPos.y);

            const char *clientStateStr = "unknown";
            yojimbo::ClientState clientState = client->yj_client->GetClientState();
            switch (clientState) {
                case yojimbo::CLIENT_STATE_ERROR:        clientStateStr = "CLIENT_STATE_ERROR"; break;
                case yojimbo::CLIENT_STATE_DISCONNECTED: clientStateStr = "CLIENT_STATE_DISCONNECTED"; break;
                case yojimbo::CLIENT_STATE_CONNECTING:   clientStateStr = "CLIENT_STATE_CONNECTING"; break;
                case yojimbo::CLIENT_STATE_CONNECTED:    clientStateStr = "CLIENT_STATE_CONNECTED"; break;
            }

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
                uiPosition.x += 16.0f;
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
                uiPosition.x -= 16.0f;
            }

            Rectangle todoListRect{};
            DRAW_TEXT_MEASURE(&todoListRect, showTodoList ? "[-] todo" : "[+] todo", "");
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, todoListRect))
            {
                showTodoList = !showTodoList;
            }
            if (showTodoList) {
                uiPosition.y += 8;
                client->todoList.Draw(uiPosition);
            }
        }

        EndDrawing();
        //yojimbo_sleep(0.001);

        // Nobody else handled it, so user probably wants to disconnect or quit
        // TODO: Show a menu when in-game
        if (escape) {
            switch (client->yj_client->GetClientState()) {
                case yojimbo::CLIENT_STATE_CONNECTED:
                case yojimbo::CLIENT_STATE_CONNECTING: {
                    client->Stop();
                    break;
                }
                default: {
                    quit = true;
                    break;
                }
            }
        }

        if (WindowShouldClose()) {
            quit = true;
        }
    }

    //--------------------
    // Cleanup
    client->Stop();
    delete client->yj_client;
    client->yj_client = {};
    delete client;
    ShutdownYojimbo();

    FreeCommon();
    CloseWindow();

    return err;
}