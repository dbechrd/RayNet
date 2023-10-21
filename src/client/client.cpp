#include "../common/collision.h"
#include "../common/data.h"
#include "../common/histogram.h"
#include "../common/io.h"
#include "../common/ui/ui.h"
#include "client_world.h"
#include "game_client.h"
#include "todo.h"

void draw_menu_main(GameClient &client, bool &quit)
{
    Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() / 2.0f) };
    uiPosition.y -= 50;
    UIStyle uiStyleMenu {};
    uiStyleMenu.margin = {};
    uiStyleMenu.pad = { 16, 4 };
    uiStyleMenu.bgColor[UI_CtrlTypeButton] = BLANK;
    uiStyleMenu.fgColor = RAYWHITE;
    uiStyleMenu.font = &fntBig;
    uiStyleMenu.alignH = TextAlign_Center;
    UI uiMenu{ uiPosition, uiStyleMenu };

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
        //rnSoundCatalog.Play(RN_Sound_Lily_Introduction);
        client.TryConnect();
    }
    uiMenu.Newline();
    uiMenu.Button("Options");
    uiMenu.Newline();
    UIState quitButton = uiMenu.Button("Quit");
    if (quitButton.released) {
        quit = true;
    }

    // Draw font atlas for SDF font
    //DrawTexture(fntBig.texture, GetRenderWidth() - fntBig.texture.width, 0, WHITE);
}

static data::Entity campfire{};

static const char *connectingStrs[] = {
    "Connecting",
    ". Connecting .",
    ".. Connecting ..",
    "... Connecting ..."
};
static int connectingDotIdx = 0;
static double connectingDotIdxLastUpdatedAt = 0;

void draw_menu_connecting(GameClient &client)
{
    if (campfire.sprite.empty()) {
        campfire.sprite = "sprite_obj_campfire";
    }

    data::UpdateSprite(campfire, client.frameDt, !connectingDotIdxLastUpdatedAt);
    if (!connectingDotIdxLastUpdatedAt) {
        connectingDotIdxLastUpdatedAt = client.now;
    } else if (client.now > connectingDotIdxLastUpdatedAt + 0.5) {
        connectingDotIdxLastUpdatedAt = client.now;
        connectingDotIdx = ((size_t)connectingDotIdx + 1) % ARRAY_SIZE(connectingStrs);
    }

    const data::GfxFrame &campfireFrame = data::GetSpriteFrame(campfire);

    UIStyle uiStyleMenu {};
    uiStyleMenu.margin = {};
    uiStyleMenu.pad = { 16, 4 };
    uiStyleMenu.bgColor[UI_CtrlTypeButton] = BLANK;
    uiStyleMenu.fgColor = RAYWHITE;
    uiStyleMenu.font = &fntBig;
    uiStyleMenu.alignH = TextAlign_Center;

    Vector2 uiPosition{ floorf(GetRenderWidth() / 2.0f), floorf(GetRenderHeight() - uiStyleMenu.font->baseSize * 4) };
    UI uiMenu{ uiPosition, uiStyleMenu };

    const Vector2 cursorScreen = uiMenu.CursorScreen();
    const Vector2 campfirePos = cursorScreen; //Vector2Subtract(cursorScreen, { (float)(campfireFrame.w / 2), 0 });
    campfire.position.x = campfirePos.x;
    campfire.position.y = campfirePos.y;
    campfire.position.z = 0;

    data::DrawSprite(campfire);
    uiMenu.Space({ 0, (float)uiStyleMenu.font->baseSize / 2 });

    if (client.yj_client->IsConnecting()) {
        uiMenu.Text(connectingStrs[connectingDotIdx]);
    } else {
        uiMenu.Text("   Loading...");
    }
    uiMenu.Newline();
}

void reset_menu_connecting(void)
{
    data::ResetSprite(campfire);
    connectingDotIdx = 0;
    connectingDotIdxLastUpdatedAt = 0;
}

void update_camera(Camera2D &camera, data::Tilemap *map, Vector2 target, float frameDt)
{
    camera.offset = {
        /*floorf(*/GetRenderWidth ()/2.0f/*)*/,
        /*floorf(*/GetRenderHeight()/2.0f/*)*/
    };

    if (!io.KeyDown(KEY_SPACE)) {
#if CL_CAMERA_LERP
        // https://www.gamedeveloper.com/programming/improved-lerp-smoothing-
        const float halfLife = 8.0f;
        float alpha = 1.0f - exp2f(-halfLife * frameDt);
        camera.target.x = LERP(camera.target.x, target.x, alpha);
        camera.target.y = LERP(camera.target.y, target.y, alpha);

#else
        camera.target.x = target.x;
        camera.target.y = target.y;
#endif
    }

    if (map) {
        Vector2 mapSize = { (float)map->width * TILE_W, map->height * (float)TILE_W };
        camera.target.x = CLAMP(camera.target.x, camera.offset.x / camera.zoom, mapSize.x - camera.offset.x / camera.zoom);
        camera.target.y = CLAMP(camera.target.y, camera.offset.y / camera.zoom, mapSize.y - camera.offset.y / camera.zoom);
    }

    // Zoom based on mouse wheel
    static float zoomTarget = camera.zoom;
    float wheel = io.MouseWheelMove();
    if (wheel != 0) {
        // Get the world point that is under the mouse
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera );

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
        const Vector2 center{ GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };
        const float radius = 128;
        DrawCircleV(center, radius, Fade(PINK, 0.7f));
        const float pieSliceDeg = 360.0f / client.hudSpinnerCount;
        const float raylibOffset = -90;
        const float angleStart = pieSliceDeg * client.hudSpinnerIndex + raylibOffset;
        const float angleEnd = angleStart + pieSliceDeg;
        DrawCircleSector(center, radius, angleStart, angleEnd, 32, RED);
    }

    UIStyle uiHUDMenuStyle{};
    UI uiHUDMenu{ {}, uiHUDMenuStyle };

    // TODO: Add Quit button to the menu at the very least
    uiHUDMenu.Button("Menu");
    uiHUDMenu.Button("Quests");
    uiHUDMenu.Button("Inventory");
    uiHUDMenu.Button("Map");
    uiHUDMenu.Newline();

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

    InitWindow(800, 600, "RayNet Client");
    //InitWindow(1920, 1017, "RayNet Client");
    SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows fucking sucks at this
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_WINDOW_MAXIMIZED);
    //SetWindowState(FLAG_FULLSCREEN_MODE);

    SetExitKey(0);  // must be called after InitWindow()

    InitAudioDevice();
    SetMasterVolume(0.24f);

    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
    }

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
    GameClient *client = new GameClient(GetTime());
    client->Start();

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

        client->frameDtSmooth = LERP(client->frameDtSmooth, client->frameDt, 0.01);
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

        //--------------------
        // Accmulate input every frame
        if (client->yj_client->IsConnected()) {
            data::Entity *localPlayer = client->world->LocalPlayer();
            if (localPlayer) {
                io.PushScope(IO::IO_GameHUD);
                io.CaptureMouse();
                client->hudSpinner = io.KeyDown(KEY_TAB);
                if (client->hudSpinner) {
                    // TODO(dlb): Just use mouse position to figure out which segment is clicked
                    float wheelMove = io.MouseWheelMove();
                    if (wheelMove) {
                        int delta = (int)CLAMP(wheelMove, -1, 1) * -1;

                        if (io.KeyDown(KEY_LEFT_SHIFT)) {
                            client->hudSpinnerCount += delta;
                            client->hudSpinnerCount = CLAMP(client->hudSpinnerCount, 1, 20);
                        } else {
                            client->hudSpinnerIndex += delta;

                            client->hudSpinnerIndex %= client->hudSpinnerCount;
                            if (client->hudSpinnerIndex < 0) client->hudSpinnerIndex += client->hudSpinnerCount;
                            assert(client->hudSpinnerIndex >= 0);
                            assert(client->hudSpinnerIndex < client->hudSpinnerCount);
                        }
                    }
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

                // TODO: Actually check hand
                bool holdingWeapon = true;
                if (holdingWeapon) {
                    client->controller.cmdAccum.fire |= io.MouseButtonDown(MOUSE_LEFT_BUTTON);
                    //client->controller.cmdAccum.fire |= io.MouseButtonPressed(MOUSE_LEFT_BUTTON);
                }

                // TODO: Actually check hand
                bool holdingShovel = true;
                if (holdingShovel) {
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
                update_camera(client->world->camera2d, client->world->LocalPlayerMap(),
                    localPlayer->ScreenPos(), client->frameDt);
            }
        }

        //--------------------
        // Draw
        BeginDrawing();
            ClearBackground(GRAYISH_BLUE);

            if (client->yj_client->IsDisconnected()) {
                draw_menu_main(*client, quit);
                reset_menu_connecting();

                // Collision debug on menu screen
                if (0) {
                    static Vector2 cir_pos{};
                    static float cir_radius = 20;
                    static Rectangle rec{ 100, 100, 64, 64 };

                    cir_pos = GetMousePosition();

                    DrawCircleLines(cir_pos.x, cir_pos.y, cir_radius, BLUE);
                    DrawRectangleLinesEx(rec, 1.0f, BLUE);

                    Manifold manifold{};
                    if (dlb_CheckCollisionCircleRec(cir_pos, cir_radius, rec, &manifold)) {
                        DrawCircleV(manifold.contact, 3, GREEN);

                        Vector2 manifold_vec = Vector2Scale(manifold.normal, manifold.depth);
                        Vector2 manifold_end{
                            manifold.contact.x + manifold_vec.x,
                            manifold.contact.y + manifold_vec.y
                        };
                        DrawLine(
                            manifold.contact.x,
                            manifold.contact.y,
                            manifold_end.x,
                            manifold_end.y,
                            ORANGE
                        );
                        DrawCircleV(manifold_end, 3, ORANGE);
                    }
                }
            } else if (client->yj_client->IsConnecting() || !client->world || !client->world->localPlayerEntityId) {
                draw_menu_connecting(*client);
            } else if (client->yj_client->IsConnected()) {
                draw_game(*client);
                reset_menu_connecting();
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
#include "client_world.cpp"
#include "game_client.cpp"
#include "todo.cpp"
