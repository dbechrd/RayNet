#include "../common/collision.h"
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

    InitWindow(800, 600, "RayNet Client");
    //InitWindow(1920, 1017, "RayNet Client");
    //SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows fucking sucks at this
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_WINDOW_MAXIMIZED);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    InitAudioDevice();

    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
    }

    Music music = musAmbientOutdoors;
    PlayMusicStream(music);

    Image icon = LoadImage("resources/client.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    {
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

        const int refreshRate = GetMonitorRefreshRate(GetCurrentMonitor());
        const float refreshDt = 1.0f / refreshRate;
        if (IsWindowState(FLAG_VSYNC_HINT) && fabsf(frameDt - refreshDt) < 0.005f) {
            frameDt = refreshDt;
        }

        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.01);
        frameStart = client->now;

        client->controller.sampleInputAccum += frameDt;
        client->netTickAccum += frameDt;
        bool doNetTick = client->netTickAccum >= SV_TICK_DT;

        // TODO: rnAudioSystem.Update();
        UpdateMusicStream(music);

        io.PushScope(IO::IO_Game);

        bool escape = IsKeyPressed(KEY_ESCAPE);

        //--------------------
        // Input
        if (IsKeyPressed(KEY_F11)) {
            bool isFullScreen = IsWindowState(FLAG_FULLSCREEN_MODE);
            if (isFullScreen) {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
            } else {
                int monitor = GetCurrentMonitor();
                int width = GetMonitorWidth(monitor);
                int height = GetMonitorHeight(monitor);
                SetWindowSize(width, height);
                SetWindowState(FLAG_FULLSCREEN_MODE);
            }
        }
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

        Camera2D &camera = client->world->camera2d;

        //--------------------
        // Accmulate input every frame
        Entity *localPlayer = client->LocalPlayer();
        if (localPlayer) {
            client->controller.cmdAccum.north |= IsKeyDown(KEY_W);
            client->controller.cmdAccum.west |= IsKeyDown(KEY_A);
            client->controller.cmdAccum.south |= IsKeyDown(KEY_S);
            client->controller.cmdAccum.east |= IsKeyDown(KEY_D);
            client->controller.cmdAccum.fire |= IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
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

        localPlayer = client->LocalPlayer();
        if (localPlayer) {
            if (client->LocalPlayerEntityId() != 1) {
                SetMasterVolume(0);
            }

            //--------------------
            // Camera
            // TODO: Move update code out of draw code and update local player's
            // position before using it to determine camera location... doh!
            {
                camera.offset = { (float)GetRenderWidth()/2, (float)GetRenderHeight()/2 };

                if (!IsKeyDown(KEY_SPACE)) {
                    Entity *target = localPlayer;
#if 0
                    target = client->world->GetEntity(9);
#endif
#if CL_CAMERA_LERP
                    // https://www.gamedeveloper.com/programming/improved-lerp-smoothing-
                    const float halfLife = 8.0f;
                    float alpha = 1.0f - exp2f(-halfLife * frameDt);
                    camera.target.x = LERP(camera.target.x, target->position.x, alpha);
                    camera.target.y = LERP(camera.target.y, target->position.y, alpha);
#else
                    camera.target.x = target->position.x;
                    camera.target.y = target->position.y;
#endif
                }

                // Zoom based on mouse wheel
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    // Get the world point that is under the mouse
                    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera );

                    // Zoom increment
                    const float zoomIncrement = 0.125f;
                    camera.zoom += (wheel * zoomIncrement * camera.zoom);
                    if (camera.zoom < zoomIncrement) camera.zoom = zoomIncrement;
                }
            }

            //--------------------
            // Draw the map
            BeginMode2D(camera);
            client->world->map.Draw(camera);

            //--------------------
            // Draw the entities
            cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            for (uint32_t entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
                Entity *entity = client->world->GetEntity(entityId);
                if (!entity) {
                    continue;
                }

                if (CL_DBG_SNAPSHOT_SHADOWS) {
                    EntityGhost &ghost = client->world->ghosts[entityId];
                    Entity ghostInstance = *entity;
                    for (int i = 0; i < ghost.snapshots.size(); i++) {
                        if (!ghost.snapshots[i].serverTime) {
                            continue;
                        }
                        ghostInstance.ApplyStateInterpolated(ghost.snapshots[i], ghost.snapshots[i], 0.0);

                        const float scalePer = 1.0f / (CL_SNAPSHOT_COUNT + 1);
                        Rectangle ghostRect = ghostInstance.GetRect();
                        ghostRect = RectShrink(ghostRect, scalePer);
                        DrawRectangleRec(ghostRect, Fade(RED, 0.2f));
                    }

                    // NOTE(dlb): These aren't actually snapshot shadows, they're client-side prediction shadows
                    if (entityId == client->LocalPlayerEntityId()) {
#if CL_CLIENT_SIDE_PREDICT
                        uint32_t lastProcessedInputCmd = 0;
                        const EntitySnapshot &latestSnapshot = ghost.snapshots.newest();
                        if (latestSnapshot.serverTime) {
                            ghostInstance.ApplyStateInterpolated(latestSnapshot, latestSnapshot, 0);
                            lastProcessedInputCmd = latestSnapshot.lastProcessedInputCmd;
                        }

                        //const double cmdAccumDt = client->now - client->controller.lastInputSampleAt;
                        for (size_t cmdIndex = 0; cmdIndex < client->controller.cmdQueue.size(); cmdIndex++) {
                            InputCmd &inputCmd = client->controller.cmdQueue[cmdIndex];
                            if (inputCmd.seq > lastProcessedInputCmd) {
                                ghostInstance.ApplyForce(inputCmd.GenerateMoveForce(ghostInstance.speed));
                                ghostInstance.Tick(SV_TICK_DT);
                                client->world->map.ResolveEntityTerrainCollisions(ghostInstance);
                                DrawRectangleRec(ghostInstance.GetRect(), Fade(GREEN, 0.2f));
                            }
                        }
#endif
#if 0
                        const double cmdAccumDt = client->now - client->controller.lastInputSampleAt;
                        if (cmdAccumDt > 0) {
                            ghostInstance.ApplyForce(client->controller.cmdAccum.GenerateMoveForce(ghostInstance.speed));
                            ghostInstance.Tick(cmdAccumDt);
                            client->world->map.ResolveEntityTerrainCollisions(ghostInstance);
                            DrawRectangleRec(ghostInstance.GetRect(), Fade(BLUE, 0.2f));
                            printf("%.3f\n", cmdAccumDt);
                        }
#endif
                    }
                }

                entity->Draw(client->now);
            }

            //--------------------
            // Draw the dialogs
            client->world->Draw();

            EndMode2D();
        } else {
            SetMasterVolume(1);

            io.PushScope(IO::IO_EditorUI);

            UIStyle uiStyleMenu {};
            uiStyleMenu.margin = {};
            uiStyleMenu.pad = { 16, 4 };
            uiStyleMenu.bgColor = BLANK;
            uiStyleMenu.fgColor = RAYWHITE;
            uiStyleMenu.font = &fntHackBold32;
            uiStyleMenu.alignH = TextAlign_Center;

            Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() / 2.0f) };
            if (client->yj_client->IsDisconnected()) {
                uiPosition.y -= 50;
            } else if (client->yj_client->IsConnecting()) {
                uiPosition.y -= 150;
            }
            UI uiMenu{ uiPosition, uiStyleMenu };

            const char *connectingStrs[] = {
                "  Connecting   ",
                "  Connecting.  ",
                "  Connecting.. ",
                "  Connecting..."
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
                uiMenu.Newline();

                static Sprite campfire{};
                if (!campfire.animationId) {
                    campfire.spritesheetId = rnSpritesheetCatalog.FindOrLoad("resources/campfire.txt");
                    campfire.animationId = 0;
                }

                Vector2 campfireSize = campfire.GetSize();
                Vector2 campfirePos = Vector2Subtract(uiMenu.CursorScreen(), { campfireSize.x / 2, 0 });
                campfire.Draw(campfirePos, 1.0f, client->now);
            } else {
#if 0
                // Draw weird squares animation
                const Vector2 screenHalfSize{ GetRenderWidth()/2.0f, GetRenderHeight()/2.0f };
                const Vector2 screenCenter{ screenHalfSize.x, screenHalfSize.y };
                for (float scale = 0.0f; scale < 1.1f; scale += 0.1f) {
                    const float modScale = fmodf(texMenuBgScale + scale, 1);
                    Rectangle menuBgRect{
                        screenCenter.x - screenHalfSize.x * modScale,
                        screenCenter.y - screenHalfSize.y * modScale,
                        GetRenderWidth() * modScale,
                        GetRenderHeight() * modScale
                    };
                    DrawRectangleLinesEx(menuBgRect, 20, BLACK);
                }
#endif
                UIState connectButton = uiMenu.Button("Play");
                if (connectButton.released) {
                    //rnSoundSystem.Play(RN_Sound_Lily_Introduction);
                    client->TryConnect();
                }
                uiMenu.Newline();
                uiMenu.Button("Options");
                uiMenu.Newline();
                UIState quitButton = uiMenu.Button("Quit");
                if (quitButton.released) {
                    quit = true;
                }

                // Draw font atlas for SDF font
                //DrawTexture(fntHackBold32.texture, GetRenderWidth() - fntHackBold32.texture.width, 0, WHITE);
            }

            io.PopScope();
        }

        // HP bar
        if (client->hoveredEntityId) {
            Entity *entity = client->world->GetEntity(client->hoveredEntityId);
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
            fpsHistogram.Push(frameDt, doNetTick ? GREEN : RAYWHITE);

            hackNewCursor.x += 8;
            hackNewCursor.y += 8;
            fpsHistogram.Draw(hackNewCursor);
            hackNewCursor.y += fpsHistogram.histoHeight + 8;
        }

        Vector2 uiPosition{ hackNewCursor };

        // Debug HUD
        {
            io.PushScope(IO::IO_HUD);

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

            DRAW_TEXT("frameDt", "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDtSmooth, frameDtSmooth * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("serverTime", "%.2f (%s%.2f)", client->ServerNow(), client->clientTimeDeltaVsServer > 0 ? "+" : "", client->clientTimeDeltaVsServer);
            DRAW_TEXT("localTime", "%.2f", client->now);
            DRAW_TEXT("window", "%d, %d", GetScreenWidth(), GetScreenHeight());
            DRAW_TEXT("render", "%d, %d", GetRenderWidth(), GetRenderHeight());
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
            bool hudHover = CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, netInfoRect);
            if (hudHover) {
                io.CaptureMouse();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    showNetInfo = !showNetInfo;
                }
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

            io.PopScope();
        }

        EndDrawing();
        if (!IsWindowState(FLAG_VSYNC_HINT)) {
            yojimbo_sleep(0.001);
        }

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

        io.PopScope();
        io.EndFrame();
    }

    //--------------------
    // Cleanup
    client->Stop();
    delete client->yj_client;
    client->yj_client = {};
    delete client;
    ShutdownYojimbo();

    FreeCommon();
    CloseAudioDevice();
    CloseWindow();

    return err;
}

#include "../common/common.cpp"
#include "client_world.cpp"
#include "game_client.cpp"
#include "todo.cpp"
