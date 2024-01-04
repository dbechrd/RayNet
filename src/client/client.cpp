#include "../common/boot_screen.h"
#include "../common/collision.h"
#include "../common/data.h"
#include "../common/histogram.h"
#include "../common/io.h"
#include "../common/perf_timer.h"
#include "../common/screen_fx.h"
#include "../common/ui/ui.h"
#include "client_world.h"
#include "game_client.h"
#include "menu.h"
#include "todo.h"

const bool IS_SERVER = false;

void draw_f3_menu(GameClient &client)
{
    IO::Scoped scope(IO::IO_F3Menu);

    //Vector2 hudCursor{ GetRenderWidth() - 360.0f - 8.0f, 8.0f };
    Vector2 hudCursor{ 8.0f, 48.0f };
    // TODO: ui.histogram(histogram);
    Vector2 histoCursor = hudCursor;
    hudCursor.y += (Histogram::histoHeight + 8) * 3;

    char buf[128];

#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) \
    { \
        snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
        dlb_DrawTextShadowEx(fntSmall, CSTRLEN(buf), hudCursor, RAYWHITE); \
        if (measureRect) { \
            Vector2 measure = dlb_MeasureTextEx(fntSmall, CSTRLEN(buf)); \
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
        Camera2D &camera = client.world->camera;
        Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        DRAW_TEXT("cursorWld", "%.f, %.f", cursorWorldPos.x, cursorWorldPos.y);
        Entity *localPlayer = client.world->LocalPlayer();
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
        DRAW_TEXT("inputSeq", "%u", client.controller.nextSeq);
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
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
#if _DEBUG
    dlb_CommonTests();
#endif

    Err err = RN_SUCCESS;
    PerfTimer t{ "Client" };

    SetTraceLogLevel(LOG_WARNING);

    {
        PerfTimer t{ "InitWindow" };
        InitWindow(1920, 1017, "RayNet Client");
        SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
        SetWindowState(FLAG_WINDOW_RESIZABLE);
        SetWindowState(FLAG_VSYNC_HINT);  // Gahhhhhh Windows sucks at this
        //SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        //SetWindowState(FLAG_FULLSCREEN_MODE);
        SetExitKey(0);  // must be called after InitWindow()

        DrawBootScreen();
    }

    {
        PerfTimer t{ "InitAudioDevice" };
        InitAudioDevice();
        SetMasterVolume(1.0f);
    }

    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    err = Init();
    if (err) {
        printf("Failed to load common resources\n");
    }

    // HACK: Fix this..
    for (Tilemap &map : pack_maps.tile_maps) {
        map = {};
#if 0
        for (auto &layer : map.layers) {
            for (auto &tile : layer) {
                tile = 0;
            }
        }
#endif
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

        // TODO: Send PLAY_BACKGROUND_MUSIC from server???
        if (client->yj_client->IsConnected()) {
            Tilemap *map = client->world->LocalPlayerMap();
            if (map && map->bg_music.size()) {
                const MusFile &mus_file = pack_assets.FindByName<MusFile>(map->bg_music);
                if (!IsMusicStreamPlaying(mus_file.music)) {
                    PlayMusicStream(mus_file.music);
                }
                UpdateMusicStream(mus_file.music);
            }
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
            Entity *localPlayer = client->world->LocalPlayer();
            if (localPlayer) {
                const bool button_up = io.KeyDown(KEY_W);
                const bool button_left = io.KeyDown(KEY_A);
                const bool button_down = io.KeyDown(KEY_S);
                const bool button_right = io.KeyDown(KEY_D);
                const bool button_primary = io.MouseButtonDown(MOUSE_BUTTON_LEFT);
                const bool button_secondary = io.MouseButtonPressed(MOUSE_BUTTON_RIGHT);

                // TODO: Use this for dismissing dialogs, updating player facing dir, etc.?
                //const bool button_any = button_up || button_left || button_down || button_right || button_primary;

                // TODO: Update facing direction elsewhere, then just get localPlayer.facing here?
                Camera2D& camera = client->world->camera;
                Vector2 cursorWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                Vector2 facing = Vector2Subtract(cursorWorldPos, localPlayer->Position2D());
                facing = Vector2Normalize(facing);

                Controller &controller = client->controller;
                InputCmd &input = controller.cmdAccum;
                input.SetFacing(facing);
                input.north |= button_up;
                input.west  |= button_left;
                input.south |= button_down;
                input.east  |= button_right;

                if (input.north ||
                    input.west  ||
                    input.south ||
                    input.east)
                {
                    localPlayer->ClearDialog();
                }

                controller.tile_hovered = false;
                controller.tile_x = 0;
                controller.tile_y = 0;

                Tilemap* map = client->world->LocalPlayerMap();
                {
                    Tilemap::Coord tile_coord{};
                    if (map) {
                        if (map->WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, tile_coord)) {
                            Tilemap::Coord player_coord{};
                            if (map->WorldToTileIndex(localPlayer->position.x, localPlayer->position.y, player_coord)) {
                                const int dist_x = abs((int)player_coord.x - (int)tile_coord.x);
                                const int dist_y = abs((int)player_coord.y - (int)tile_coord.y);
                                if (dist_x <= SV_MAX_TILE_INTERACT_DIST_IN_TILES &&
                                    dist_y <= SV_MAX_TILE_INTERACT_DIST_IN_TILES)
                                {
                                    controller.tile_hovered = true;
                                    controller.tile_x = tile_coord.x;
                                    controller.tile_y = tile_coord.y;
                                }
                            } else {
                                // Player somehow not on a tile!? Server error!?!?
                                assert(!"player outside of map, wot?");
                            }
                        }
                    }
                }

                const char *holdingItem = client->world->spinner.ItemName();
                if (button_primary) {
                    if (holdingItem == "Fireball") {
                        input.fire = true;
                    } else if (holdingItem == "Shovel" && controller.tile_hovered) {
                        client->SendTileInteract(map->id, controller.tile_x, controller.tile_y, true);
                    }
                }

                if (button_secondary && controller.tile_hovered) {
                    client->SendTileInteract(map->id, controller.tile_x, controller.tile_y, false);
                }
            }
        }

        //--------------------
        // Networking
        if (doNetTick) {
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
            Entity *localPlayer = client->world->LocalPlayer();
            if (localPlayer) {
                // Update world
                client->world->Update(*client);
            }
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
            //ClearBackground(BLACK); //GRAYISH_BLUE);
            ClearBackground({ 93, 137, 129, 255 });

            if (client->yj_client->IsConnected()) {
                client->world->Draw(*client);
            }

            bool back = false;
            client->menu_system.Draw(*client, back);
            if (client->menu_system.active_menu_id == Menu::MENU_MAIN && back) {
                quit = true;
            }

            if (client->menu_system.active_menu_id) {
                static double boot_fade_menu_in = client->now;
                ScreenFX_Fade(boot_fade_menu_in, CL_MENU_FADE_IN_DURATION, client->now);
            }

            if (client->showF3Menu) {
                draw_f3_menu(*client);
            }

#if _DEBUG && 0
            {
                // Debug circle-rect collision
                Vector2 center = Vector2Subtract(GetMousePosition(), { 64, 64 });
                float radius = 16.0f;

                Rectangle r{ 100, 100, 64, 64 };
                Manifold manifold{};

                Color col = DARKGRAY;
                if (dlb_CheckCollisionCircleRec(center, radius, r, &manifold)) {
                    col = MAROON;
                }

                DrawCircle(center.x, center.y, radius, DARKGRAY);
                DrawRectangleLinesEx(r, 1, col);
                DrawLineEx(manifold.contact, Vector2Add(manifold.contact, Vector2Scale(manifold.normal, manifold.depth)), 1, GREEN);
            }
#endif
#if _DEBUG && 0
            {
                // Debug circle-edge collision
                Vector2 center = Vector2Subtract(GetMousePosition(), { 64, 64 });
                float radius = 16.0f;

                Edge edge{ LineSegment2{ Vector2{ 100, 100 }, Vector2{ 200, 100 }}};
                Manifold manifold{};

                Color col = DARKGRAY;
                if (dlb_CheckCollisionCircleEdge(center, radius, edge, &manifold)) {
                    col = MAROON;
                }

                DrawCircle(center.x, center.y, radius, DARKGRAY);
                edge.Draw(MAGENTA, 2);
                DrawLineEx(manifold.contact, Vector2Add(manifold.contact, Vector2Scale(manifold.normal, manifold.depth)), 2, GREEN);
            }
#endif
#if _DEBUG && 0
            {
                // Debug circle-edge collision
                Vector2 center = { GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };
                float radius = 100.0f;

                Vector2 mouse = GetMousePosition();
                Vector2 toMouse = Vector2Subtract(mouse, center);
                uint8_t facing = InputCmd::QuantizeFacing(Vector2Normalize(toMouse));
                Vector2 dir = InputCmd::ReconstructFacing(facing);

                DrawCircle(center.x, center.y, radius, DARKGRAY);
                DrawLineEx(center, Vector2Add(center, toMouse), 2, WHITE);
                DrawLineEx(center, Vector2Add(center, Vector2Scale(dir, Vector2Length(toMouse))), 2, GREEN);
                const char *text = TextFormat("%u", facing);
                dlb_DrawTextShadowEx(fntSmall, CSTRLEN(text), { mouse.x + 10, mouse.y + 10 }, WHITE);
            }
#endif
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

    Free();
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