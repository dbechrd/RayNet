#include "../common/boot_screen.h"
#include "../common/collision.h"
#include "../common/data.h"
#include "../common/histogram.h"
#include "../common/io.h"
#include "../common/ui/ui.h"
#include "client_world.h"
#include "game_client.h"
#include "menu.h"
#include "todo.h"

void update_camera(Camera2D &camera, data::Tilemap &map, data::Entity &target, float frameDt)
{
    Vector2 targetPos = target.ScreenPos();

    camera.offset = {
        /*floorf(*/GetRenderWidth ()/2.0f/*)*/,
        /*floorf(*/GetRenderHeight()/2.0f/*)*/
    };

    if (!io.KeyDown(KEY_SPACE)) {
#if CL_CAMERA_LERP
        // https://www.gamedeveloper.com/programming/improved-lerp-smoothing-
        const float halfLife = 8.0f;
        float alpha = 1.0f - exp2f(-halfLife * frameDt);
        camera.target.x = LERP(camera.target.x, targetPos.x, alpha);
        camera.target.y = LERP(camera.target.y, targetPos.y, alpha);

#else
        camera.target.x = target.x;
        camera.target.y = target.y;
#endif
    }

    // Some maps are too small for this to work.. so I just disabled it for now
    //if (map) {
        //Vector2 mapSize = { (float)map->width * TILE_W, map->height * (float)TILE_W };
        //camera.target.x = CLAMP(camera.target.x, camera.offset.x / camera.zoom, mapSize.x - camera.offset.x / camera.zoom);
        //camera.target.y = CLAMP(camera.target.y, camera.offset.y / camera.zoom, mapSize.y - camera.offset.y / camera.zoom);
    //}

    // Snap camera when new entity target, or target entity changes maps
    static uint32_t last_target_id = 0;
    static std::string last_target_map = "";
    if (target.id != last_target_id || target.map_id != last_target_map) {
        camera.target.x = targetPos.x;
        camera.target.y = targetPos.y;
        last_target_id = target.id;
        last_target_map = target.map_id;
    }

    // Zoom based on mouse wheel
    static float zoomTarget = camera.zoom;
    float wheel = io.MouseWheelMove();
    if (wheel != 0) {
        // Get the world point that is under the mouse
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        // Zoom increment
        const float zoomIncrement = 0.1f;
        zoomTarget += (wheel * zoomIncrement * zoomTarget);
        zoomTarget = MAX(zoomIncrement, zoomTarget);
    }
    if (fabsf(camera.zoom - zoomTarget) > 0.001f) {
        const float halfLife = 8.0f;
        float alpha = 1.0f - exp2f(-halfLife * frameDt);
        camera.zoom = LERP(camera.zoom, zoomTarget, alpha);
    }

    if (io.KeyPressed(KEY_KP_1)) {
        zoomTarget = 1;
    } else if (io.KeyPressed(KEY_KP_2)) {
        zoomTarget = 2;
    }

    //camera.target.x = floorf(camera.target.x);
    //camera.target.y = floorf(camera.target.y);
    //camera.offset.x = floorf(camera.offset.x);
    //camera.offset.y = floorf(camera.offset.y);
}

void draw_game(GameClient &client)
{
    Vector2 screenSize{
        floorf((float)GetRenderWidth()),
        floorf((float)GetRenderHeight())
    };
    SetShaderValue(shdPixelFixer, shdPixelFixerScreenSizeUniformLoc, &screenSize, SHADER_UNIFORM_VEC2);

    client.world->Draw(client);

    //--------------------
    // Draw in-game menu
    io.PushScope(IO::IO_GameHUD);
    //io.CaptureMouse();

    if (client.hudSpinner) {
        const float innerRadius = 80;
        const float outerRadius = innerRadius * 2;
        const float centerRadius = innerRadius + (outerRadius - innerRadius) / 2;

        Vector2 mousePos = GetMousePosition();
        if (!client.hudSpinnerPrev) {
            client.hudSpinnerPos = mousePos; //{ GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };
            CircleConstrainToScreen(client.hudSpinnerPos, outerRadius);
        }

        Vector2 toMouse = Vector2Subtract(mousePos, client.hudSpinnerPos);
        float toMouseLen2 = Vector2LengthSqr(toMouse);

        if (toMouseLen2 >= innerRadius*innerRadius) {
            // Position on circle, normalized to 0.0 - 1.0 range, clockwise from 12 o'clock
            float pieAlpha = 1.0f - (atan2f(toMouse.x, toMouse.y) / PI + 1.0f) / 2.0f;
            client.hudSpinnerIndex = pieAlpha * client.hudSpinnerCount;
            client.hudSpinnerIndex = CLAMP(client.hudSpinnerIndex, 0, client.hudSpinnerCount);
        }

#if 1
        // TODO(cleanup): Debug code to increase/decrease # of pie entries
        float wheelMove = io.MouseWheelMove();
        if (wheelMove) {
            int delta = (int)CLAMP(wheelMove, -1, 1) * -1;
            client.hudSpinnerCount += delta;
            client.hudSpinnerCount = CLAMP(client.hudSpinnerCount, 1, 20);
        }
#endif

        const float pieSliceDeg = 360.0f / client.hudSpinnerCount;
        const float angleStart = pieSliceDeg * client.hudSpinnerIndex - 90;
        const float angleEnd = angleStart + pieSliceDeg;
        const Color color = ColorFromHSV(angleStart, 0.7f, 0.7f);

        //DrawCircleV(client.hudSpinnerPos, outerRadius, Fade(LIGHTGRAY, 0.6f));
        DrawRing(client.hudSpinnerPos, innerRadius, outerRadius, 0, 360, 32, Fade(LIGHTGRAY, 0.6f));

        //DrawCircleSector(client.hudSpinnerPos, outerRadius, angleStart, angleEnd, 32, Fade(SKYBLUE, 0.6f));
        DrawRing(client.hudSpinnerPos, innerRadius, outerRadius, angleStart, angleEnd, 32, Fade(color, 0.8f));

        for (int i = 0; i < client.hudSpinnerCount; i++) {
            // Find middle of pie slice as a percentage of total pie circumference
            const float iconPieAlpha = (float)i / client.hudSpinnerCount - 0.25f + (1.0f / client.hudSpinnerCount * 0.5f);
            const Vector2 iconCenter = {
                client.hudSpinnerPos.x + centerRadius * cosf(2 * PI * iconPieAlpha),
                client.hudSpinnerPos.y + centerRadius * sinf(2 * PI * iconPieAlpha)
            };

            const char *menuText = i < ARRAY_SIZE(client.hudSpinnerItems) ? client.hudSpinnerItems[i] : "-Empty-";

            //DrawCircle(iconCenter.x, iconCenter.y, 2, BLACK);
            Vector2 textSize = dlb_MeasureTextEx(fntMedium, menuText, strlen(menuText));
            Vector2 textPos{
                iconCenter.x - textSize.x / 2.0f,
                iconCenter.y - textSize.y / 2.0f
            };
            //DrawTextShadowEx(fntMedium, TextFormat("%d %.2f", i, iconPieAlpha), iconCenter, RED);
            DrawTextShadowEx(fntMedium, menuText, textPos, WHITE);
        }

#if 0
        // TODO(cleanup): Draw debug text on pie menu
        DrawTextShadowEx(fntMedium,
            TextFormat("%.2f (%d of %d)", pieAlpha, client.hudSpinnerIndex + 1, client.hudSpinnerCount),
            center, WHITE
        );
#endif
    }

    UIStyle uiHUDMenuStyle{};
    UI uiHUDMenu{ {}, uiHUDMenuStyle };

    // TODO: Add Quit button to the menu at the very least
    uiHUDMenu.Button("Menu");
    uiHUDMenu.Button("Quests");
    uiHUDMenu.Button("Inventory");
    uiHUDMenu.Button("Map");
    uiHUDMenu.Newline();

    const char *holdingItem = client.HudSpinnerItemName();
    if (!holdingItem) {
        holdingItem = "-Empty-";
    }
    uiHUDMenu.Text(holdingItem);

    io.PopScope();
}

void draw_f3_menu(GameClient &client)
{
    io.PushScope(IO::IO_F3Menu);

    //Vector2 hudCursor{ GetRenderWidth() - 360.0f - 8.0f, 8.0f };
    Vector2 hudCursor{ 8.0f, 48.0f };
    // TODO: ui.histogram(histogram);
    Vector2 histoCursor = hudCursor;
    hudCursor.y += (Histogram::histoHeight + 8) * 3;

    char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                snprintf(buf, sizeof(buf), "%-11s : " fmt, label, __VA_ARGS__); \
                DrawTextShadowEx(fntSmall, buf, hudCursor, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntSmall, buf, (float)fntSmall.baseSize, 1.0); \
                    *measureRect = { hudCursor.x, hudCursor.y, measure.x, measure.y }; \
                } \
                hudCursor.y += fntSmall.baseSize; \
            }

#define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

    DRAW_TEXT("frameDt", "%.2f fps (%.2f ms) (vsync=%s)",
        1.0 / client.frameDtSmooth,
        client.frameDtSmooth * 1000.0,
        IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off"
    );
    DRAW_TEXT("serverTime", "%.2f (%s%.2f)",
        client.ServerNow(),
        client.clientTimeDeltaVsServer > 0 ? "+" : "", client.clientTimeDeltaVsServer
    );
    DRAW_TEXT("localTime", "%.2f", client.now);
    DRAW_TEXT("window", "%d, %d", GetScreenWidth(), GetScreenHeight());
    DRAW_TEXT("render", "%d, %d", GetRenderWidth(), GetRenderHeight());
    DRAW_TEXT("cursorScn", "%d, %d", GetMouseX(), GetMouseY());
    if (client.yj_client->IsConnected() && client.world) {
        Camera2D &camera = client.world->camera2d;
        Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        DRAW_TEXT("cursorWld", "%.f, %.f", cursorWorldPos.x, cursorWorldPos.y);
        data::Entity *localPlayer = client.world->LocalPlayer();
        if (localPlayer) {
            DRAW_TEXT("player", "%.2f, %.2f", localPlayer->position.x, localPlayer->position.y);
        }
    }

    const char *clientStateStr = "unknown";
    yojimbo::ClientState clientState = client.yj_client->GetClientState();
    switch (clientState) {
        case yojimbo::CLIENT_STATE_ERROR:        clientStateStr = "CLIENT_STATE_ERROR"; break;
        case yojimbo::CLIENT_STATE_DISCONNECTED: clientStateStr = "CLIENT_STATE_DISCONNECTED"; break;
        case yojimbo::CLIENT_STATE_CONNECTING:   clientStateStr = "CLIENT_STATE_CONNECTING"; break;
        case yojimbo::CLIENT_STATE_CONNECTED:    clientStateStr = "CLIENT_STATE_CONNECTED"; break;
    }

    Rectangle netInfoRect{};
    DRAW_TEXT_MEASURE(&netInfoRect,
        client.showNetInfo ? "[-] state" : "[+] state",
        "%s", clientStateStr
    );
    bool hudHover = CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, netInfoRect);
    if (hudHover) {
        io.CaptureMouse();
        if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            client.showNetInfo = !client.showNetInfo;
        }
    }
    if (client.showNetInfo) {
        hudCursor.x += 16.0f;
        yojimbo::NetworkInfo netInfo{};
        client.yj_client->GetNetworkInfo(netInfo);
        DRAW_TEXT("rtt", "%.2f", netInfo.RTT);
        DRAW_TEXT("% loss", "%.2f", netInfo.packetLoss);
        DRAW_TEXT("sent (kbps)", "%.2f", netInfo.sentBandwidth);
        DRAW_TEXT("recv (kbps)", "%.2f", netInfo.receivedBandwidth);
        DRAW_TEXT("ack  (kbps)", "%.2f", netInfo.ackedBandwidth);
        DRAW_TEXT("sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
        DRAW_TEXT("recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
        DRAW_TEXT("ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
        hudCursor.x -= 16.0f;
    }

    Rectangle todoListRect{};
    DRAW_TEXT_MEASURE(&todoListRect, client.showTodoList ? "[-] todo" : "[+] todo", "");
    if (CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, todoListRect)) {
        io.CaptureMouse();
        if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            client.showTodoList = !client.showTodoList;
        }
    }
    if (client.showTodoList) {
        hudCursor.y += 8;
        client.todoList.Draw(hudCursor);
    }

    histoFps.Draw(histoCursor);
    histoCursor.y += Histogram::histoHeight + 8;
    histoInput.Draw(histoCursor);
    histoCursor.y += Histogram::histoHeight + 8;
    histoDx.Draw(histoCursor);
    histoCursor.y += Histogram::histoHeight + 8;

    histoFps.DrawHover();
    histoInput.DrawHover();
    histoDx.DrawHover();

    io.PopScope();
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
#if _DEBUG
    dlb_CommonTests();
#endif

    Err err = RN_SUCCESS;

    //SetTraceLogLevel(LOG_WARNING);

    InitWindow(1920, 1017, "RayNet Client");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    //SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows sucks at this
    //SetWindowState(FLAG_FULLSCREEN_MODE);
    SetExitKey(0);  // must be called after InitWindow()

    DrawBootScreen();

    InitAudioDevice();
    SetMasterVolume(0.5f);

    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
    }

    Image icon = LoadImage("../res/client.png");
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
    GameClient *client = new GameClient(GetTime());
    client->Start();
    client->menu_system.TransitionTo(Menu::MENU_MAIN);

    static float texMenuBgScale = 0;

    bool quit = false;
    while (!quit) {
        client->frame++;
        client->now = GetTime();
        client->frameDt = MIN(client->now - client->frameStart, SV_TICK_DT);  // arbitrary limit for now

        const int refreshRate = GetMonitorRefreshRate(GetCurrentMonitor());
        const float refreshDt = 1.0f / refreshRate;
        if (IsWindowState(FLAG_VSYNC_HINT) && fabsf(client->frameDt - refreshDt) < 0.005f) {
            client->frameDt = refreshDt;
        }

        client->frameDtSmooth = LERP(client->frameDtSmooth, client->frameDt, 0.1);
        client->frameStart = client->now;

        client->controller.sampleInputAccum += client->frameDt;
        client->netTickAccum += client->frameDt;
        bool doNetTick = client->netTickAccum >= SV_TICK_DT;

        // TODO: Move this somewhere in client?
        if (client->yj_client->IsConnected()) {
            const data::MusFile &mus_file = data::packs[0]->FindMusic(client->world->musBackgroundMusic);
            if (!IsMusicStreamPlaying(mus_file.music)) {
                PlayMusicStream(mus_file.music);
            }
            UpdateMusicStream(mus_file.music);
        }

        //--------------------
        // Input
        io.PushScope(IO::IO_Game);

        bool escape = io.KeyPressed(KEY_ESCAPE);

        // Global Input (ignores io stack; only for function keys)
        if (io.KeyPressed(KEY_F11)) {
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
        if (io.KeyPressed(KEY_F3)) {
            client->showF3Menu = !client->showF3Menu;
        }

        // Game input (IO layers with higher precedence can steal this input)
        if (io.KeyPressed(KEY_V)) {
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
        if (io.KeyPressed(KEY_C)) {
            client->TryConnect();
        }
        if (io.KeyPressed(KEY_X)) {
            client->Stop();
        }
        if (io.KeyPressed(KEY_Z)) {
            if (client->yj_client->IsConnected()) {
                client->world->showSnapshotShadows = !client->world->showSnapshotShadows;
            }
        }
        if (io.KeyPressed(KEY_T)) {
            client->showTodoList = !client->showTodoList;
            if (client->showTodoList) {
                client->todoList.Load(TODO_LIST_PATH);
            }
        }
        if (io.KeyPressed(KEY_H)) {
            // TODO: fpsHistogram.update() which checks input and calls TogglePause?
            Histogram::paused = !Histogram::paused;
        }
        if (io.KeyPressed(KEY_F)) {
            client->fadeDirection = -1;
            client->fadeDuration = SV_WARP_FADE_DURATION;
            client->fadeValue = 1;
        }

        //--------------------
        // Accmulate input every frame
        if (client->yj_client->IsConnected()) {
            data::Entity *localPlayer = client->world->LocalPlayer();
            if (localPlayer) {
                io.PushScope(IO::IO_GameHUD);
                client->hudSpinnerPrev = client->hudSpinner;
                client->hudSpinner = io.KeyDown(KEY_TAB);
                if (client->hudSpinner) {
                    io.CaptureMouse();
                }
                io.PopScope();

                // TODO: Update facing direction elsewhere, then just get localPlayer.facing here?
                Camera2D& camera = client->world->camera2d;
                Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                Vector2 facing = Vector2Subtract(cursorWorldPos, localPlayer->ScreenPos());
                facing = Vector2Normalize(facing);
                client->controller.cmdAccum.facing = facing;

                client->controller.cmdAccum.north |= io.KeyDown(KEY_W);
                client->controller.cmdAccum.west |= io.KeyDown(KEY_A);
                client->controller.cmdAccum.south |= io.KeyDown(KEY_S);
                client->controller.cmdAccum.east |= io.KeyDown(KEY_D);

                // TODO: Make function
                const char *holdingItem = client->HudSpinnerItemName();

                // TODO: Actually check hand
                if (holdingItem == "Fireball") {
                    client->controller.cmdAccum.fire |= io.MouseButtonDown(MOUSE_LEFT_BUTTON);
                    //client->controller.cmdAccum.fire |= io.MouseButtonPressed(MOUSE_LEFT_BUTTON);
                }

                // TODO: Actually check hand
                if (holdingItem == "Shovel") {
                    if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        data::Tilemap::Coord coord{};
                        data::Tilemap* map = client->world->LocalPlayerMap();
                        if (map && map->WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord)) {
                            // TOOD: Send mapId too then validate server-side
                            client->SendTileInteract(map->id, coord.x, coord.y);
                        }
                    }
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
        Histogram::Entry histoEntry{ client->frame, client->now };
        histoInput.Push(histoEntry);
        histoDx.Push(histoEntry);
        histoEntry.value = client->frameDt * 1000.0f;
        histoEntry.color = doNetTick ? DARKPURPLE : RAYWHITE;
        histoFps.Push(histoEntry);

        if (client->yj_client->IsConnected()) {
            data::Entity *localPlayer = client->world->LocalPlayer();
            if (localPlayer) {
                // Update world
                client->world->Update(*client);
                // Update camera
                update_camera(client->world->camera2d, *client->world->LocalPlayerMap(),
                    *localPlayer, client->frameDt);
            }
        }

        if (client->fadeDuration) {
            client->fadeValue += (1.0f / (client->fadeDuration * 0.5f)) * client->fadeDirection * client->frameDt;
            if (client->fadeDirection < 0 && client->fadeValue < 0) {
                client->fadeDirection = 1;
                client->fadeValue = 0;
            } else if (client->fadeDirection > 0 && client->fadeValue > 1) {
                client->fadeDirection = 0;
                client->fadeDuration = 0;
                client->fadeValue = 0;
            }
            printf("now = %f, fadeAccum = %f\n", client->now, client->fadeValue);
        }

        if (client->yj_client->IsDisconnected()) {
            client->menu_system.TransitionTo(Menu::MENU_MAIN);
        } else if (client->yj_client->IsConnecting() || !client->world || !client->world->localPlayerEntityId) {
            client->menu_system.TransitionTo(Menu::MENU_CONNECTING);
        } else if (client->yj_client->IsConnected()) {
            client->menu_system.TransitionTo(Menu::MENU_NONE);
        }

        //--------------------
        // Draw
        BeginDrawing();
            ClearBackground(BLACK); //GRAYISH_BLUE);

            if (client->yj_client->IsConnected()) {
                draw_game(*client);
            }

            if (client->fadeDuration) {
                DrawRectangle(0, 0, GetRenderWidth(), GetRenderHeight(), Fade(BLACK, 1.0f - client->fadeValue));
            }

            bool back = false;
            client->menu_system.Draw(*client, back);
            if (client->menu_system.active_menu_id == Menu::MENU_MAIN && back) {
                quit = true;
            }

            if (client->showF3Menu) {
                draw_f3_menu(*client);
            }
        EndDrawing();

        if (!IsWindowState(FLAG_VSYNC_HINT)) {
            //yojimbo_sleep(0.001);
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
#include "../common/boot_screen.cpp"
#include "client_world.cpp"
#include "game_client.cpp"
#include "menu.cpp"
#include "todo.cpp"