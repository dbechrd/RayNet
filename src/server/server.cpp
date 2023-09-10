#include "../common/collision.h"
#include "../common/histogram.h"
#include "../common/io.h"
#include "../common/ui/ui.h"
#include "editor.h"
#include "game_server.h"

void RN_TraceLogCallback(int logLevel, const char *text, va_list args)
{
    //return;

    const char *outofbounds = "Requested image pixel";
    if (!strncmp(text, "Requested image pixel", strlen(outofbounds))) {
        logLevel = LOG_FATAL;
    }

    // Message has level below current threshold, don't emit
    //if (logType < TraceLogLevel) return;

    char buffer[128] = { 0 };

    switch (logLevel)
    {
        case LOG_TRACE: strcpy(buffer, "TRACE: "); break;
        case LOG_DEBUG: strcpy(buffer, "DEBUG: "); break;
        case LOG_INFO: strcpy(buffer, "INFO: "); break;
        case LOG_WARNING: strcpy(buffer, "WARNING: "); break;
        case LOG_ERROR: strcpy(buffer, "ERROR: "); break;
        case LOG_FATAL: strcpy(buffer, "FATAL: "); break;
        default: break;
    }

    strcat(buffer, text);
    strcat(buffer, "\n");
    vprintf(buffer, args);
    fflush(stdout);

    if (logLevel > LOG_WARNING) {
        assert(!"raylib is sad");
    }

    // If fatal logging, exit program
    if (logLevel == LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void UpdateCamera(Camera2D &camera)
{
    if (io.MouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);

        camera.target = Vector2Add(camera.target, delta);
    }

    // Zoom based on mouse wheel
    float wheel = io.MouseWheelMove();
    if (wheel != 0) {
        // Get the world point that is under the mouse
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        // Set the offset to where the mouse is
        camera.offset = GetMousePosition();

        // Set the target to match, so that the camera maps the world space point
        // under the cursor to the screen space point under the cursor at any zoom
        camera.target = mouseWorldPos;

        // Zoom increment
        const float zoomIncrement = 0.125f;

        camera.zoom += (wheel * zoomIncrement * camera.zoom);
        if (camera.zoom < zoomIncrement) camera.zoom = zoomIncrement;
    }

    if (io.KeyPressed(KEY_KP_1)) {
        camera.zoom = 1;
    } else if (io.KeyPressed(KEY_KP_2)) {
        camera.zoom = 2;
    }
}

void draw_f3_menu(GameServer &server, Camera2D &camera)
{
    io.PushScope(IO::IO_F3Menu);

    Vector2 hudCursor{
        GetRenderWidth() - 360.0f - 8.0f,
        8.0f
    };

    Vector2 histoCursor = hudCursor;
    hudCursor.y += (Histogram::histoHeight + 8) * 1;

    char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
            if (label) { \
                snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
            } else { \
                snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
            } \
            DrawTextShadowEx(fntSmall, buf, hudCursor, RAYWHITE); \
            if (measureRect) { \
                Vector2 measure = MeasureTextEx(fntSmall, buf, (float)fntSmall.baseSize, 1.0); \
                *measureRect = { hudCursor.x,hudCursor.y, measure.x, measure.y }; \
            } \
            hudCursor.y += fntSmall.baseSize; \
        }

#define DRAW_TEXT(label, fmt, ...) \
        DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

    DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)",
        1.0 / server.frameDt,
        server.frameDt * 1000.0,
        IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off"
    );
    DRAW_TEXT("time", "%.02f", server.yj_server->GetTime());
    DRAW_TEXT("tick", "%" PRIu64, server.tick);
    DRAW_TEXT("tickAccum", "%.02f", server.tickAccum);
    DRAW_TEXT("window", "%d, %d", GetScreenWidth(), GetScreenHeight());
    DRAW_TEXT("render", "%d, %d", GetRenderWidth(), GetRenderHeight());
    DRAW_TEXT("cursorScn", "%d, %d", GetMouseX(), GetMouseY());
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
    DRAW_TEXT("cursorWld", "%.f, %.f", cursorWorldPos.x, cursorWorldPos.y);
    DRAW_TEXT("clients", "%d", server.yj_server->GetNumConnectedClients());

    static bool showClientInfo[yojimbo::MaxClients];
    for (int clientIdx = 0; clientIdx < yojimbo::MaxClients; clientIdx++) {
        if (!server.yj_server->IsClientConnected(clientIdx)) {
            continue;
        }

        Rectangle clientRowRect{};
        DRAW_TEXT_MEASURE(&clientRowRect,
            showClientInfo[clientIdx] ? "[-] client" : "[+] client",
            "%d", clientIdx
        );

        bool hudHover = CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, clientRowRect);
        if (hudHover) {
            io.CaptureMouse();
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                showClientInfo[clientIdx] = !showClientInfo[clientIdx];
            }
        }
        if (showClientInfo[clientIdx]) {
            hudCursor.x += 16.0f;
            yojimbo::NetworkInfo netInfo{};
            server.yj_server->GetNetworkInfo(clientIdx, netInfo);
            DRAW_TEXT("  rtt", "%.02f", netInfo.RTT);
            DRAW_TEXT("  % loss", "%.02f", netInfo.packetLoss);
            DRAW_TEXT("  sent (kbps)", "%.02f", netInfo.sentBandwidth);
            DRAW_TEXT("  recv (kbps)", "%.02f", netInfo.receivedBandwidth);
            DRAW_TEXT("  ack  (kbps)", "%.02f", netInfo.ackedBandwidth);
            DRAW_TEXT("  sent (pckt)", "%" PRIu64, netInfo.numPacketsSent);
            DRAW_TEXT("  recv (pckt)", "%" PRIu64, netInfo.numPacketsReceived);
            DRAW_TEXT("  ack  (pckt)", "%" PRIu64, netInfo.numPacketsAcked);
            hudCursor.x -= 16.0f;
        }
    }

    histoFps.Draw(histoCursor);
    histoCursor.y += Histogram::histoHeight + 8;
    //histoInput.Draw(histoCursor);
    //histoCursor.y += Histogram::histoHeight + 8;
    //histoDx.Draw(histoCursor);
    //histoCursor.y += Histogram::histoHeight + 8;

    histoFps.DrawHover();
    //histoInput.DrawHover();
    //histoDx.DrawHover();

    io.PopScope();
}

Err Play(GameServer &server)
{
    Err err = RN_SUCCESS;

    Camera2D camera{};
    camera.zoom = 1;

    // TODO: Store name, pointer is unstable if vector resizes. Or reserve() the vector.
    Editor editor{ LEVEL_001 };
    editor.Init();

    bool quit = false;
    while (!quit) {
        io.PushScope(IO::IO_Game);

        server.frame++;
        server.now = GetTime();
        server.frameDt = MIN(server.now - server.frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        server.frameDtSmooth = LERP(server.frameDtSmooth, server.frameDt, 0.1);
        server.frameStart = server.now;

        server.tickAccum += server.frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;

        // Global Input (ignores io stack; only for function keys)
        if (IsKeyPressed(KEY_F3)) {
            server.showF3Menu = !server.showF3Menu;
        }
        if (IsKeyPressed(KEY_F11)) {
            bool isFullScreen = IsWindowState(FLAG_FULLSCREEN_MODE);
            if (isFullScreen) {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
            } else {
                SetWindowState(FLAG_FULLSCREEN_MODE);
            }
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
        if (io.KeyPressed(KEY_H)) {
            Histogram::paused = !Histogram::paused;
        }
        if (io.KeyPressed(KEY_M)) {
            //server.map =
        }

        editor.HandleInput(camera);

        UpdateCamera(camera);

        Histogram::Entry histoEntry{ server.frame, server.now };
        histoEntry.value = server.frameDt * 1000.0f;
        histoEntry.color = doNetTick ? DARKPURPLE : RAYWHITE;
        histoFps.Push(histoEntry);

        while (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            server.Update();
        }

        //--------------------
        // Draw
        BeginDrawing();
            ClearBackground(BLUE_DESAT);
            BeginMode2D(camera);
                // [World] Tilemap
                auto &editor_map = data::packs[0]->FindTileMap(editor.map_name);
                editor_map.Draw(camera);

                // [Editor] Overlays
                editor.DrawGroundOverlays(camera, server.now);

                // [World] Entities
                // NOTE(dlb): We could build an array of { entityIndex, position.y } and sort it
                // each frame, then render the entities in that order.
                for (data::Entity &entity : entityDb->entities) {
                    if (entity.map_name == editor_map.name) {
                        entityDb->DrawEntity(entity.id);
                    }
                }

                // [Editor] Overlays
                editor.DrawEntityOverlays(camera, server.now);

                // [Debug] Last collision
                DrawRectangleLinesEx(lastCollisionA, 1, RED);
                DrawRectangleLinesEx(lastCollisionB, 1, GREEN);
            EndMode2D();

            // [Editor] Menus, action bar, etc.
            editor.DrawUI({}, server, server.now);

            static int lastSentTestId = 0;
            if (editor.state.entities.testId > lastSentTestId) {
                server.BroadcastEntityDespawnTest(editor.state.entities.testId);
                lastSentTestId = editor.state.entities.testId;
            }

            // [Debug] FPS, clock, etc.
            if (server.showF3Menu) {
                draw_f3_menu(server, camera);
            }
        EndDrawing();  // note has to be last thing in loop

        yojimbo_sleep(0.001);

        if (WindowShouldClose() || io.KeyPressed(KEY_ESCAPE)) {
            // Nobody else handled it, so user probably wants to quit
            quit = true;
        }

        io.PopScope();
        io.EndFrame();
    }

    return err;
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    Err err = RN_SUCCESS;

    GameServer *server{};

    do {
        //SetTraceLogLevel(LOG_WARNING);
        SetTraceLogCallback(RN_TraceLogCallback);

        InitWindow(1920, 1017, "RayNet Server");
        //SetWindowState(FLAG_VSYNC_HINT);  // KEEP THIS ENABLED it makes the room cooler
        SetWindowState(FLAG_WINDOW_RESIZABLE);
        SetWindowState(FLAG_WINDOW_MAXIMIZED);
        SetExitKey(0);  // must be called after InitWindow()

        InitAudioDevice();
        SetMasterVolume(0);

        // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
        srand((unsigned int)GetTime());

        err = InitCommon();
        if (err) {
            printf("Failed to load common resources\n");
            break;
        }

        Image icon = LoadImage("resources/server.png");
        SetWindowIcon(icon);
        UnloadImage(icon);

        Vector2 screenSize = { (float)GetRenderWidth(), (float)GetRenderHeight() };

    #if CL_DBG_ONE_SCREEN
        const int monitorWidth = GetMonitorWidth(0);
        const int monitorHeight = GetMonitorHeight(0);
        SetWindowPosition(
            monitorWidth / 2 - (int)screenSize.x, // / 2,
            monitorHeight / 2 - (int)screenSize.y / 2
        );
    #elif CL_DBG_TWO_SCREEN
        const int monitorWidth = GetMonitorWidth(1);
        const int monitorHeight = GetMonitorHeight(1);
        Vector2 monitor2 = GetMonitorPosition(0);
        SetWindowPosition(
            monitor2.x + monitorWidth / 2 - (int)screenSize.x / 2,
            monitor2.y + monitorHeight / 2 - (int)screenSize.y / 2
        );
    #endif

        double now = GetTime();

        //--------------------
        // Create server
        // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
        server = new GameServer(now);
        if (!server) {
            printf("error: failed to allocate server\n");
            err = RN_BAD_ALLOC;
            break;
        }

        //--------------------
        // Load necessary maps
        if (!server->FindOrLoadMap(LEVEL_001)) {
            err = RN_BAD_FILE_READ;
            break;
        }
        //if (!server->FindOrLoadMap(LEVEL_002)) break;

        //--------------------
        // Start the server
        err = server->Start();
        if (err) {
            printf("error: failed to start server\n");
            break;
        }

        //--------------------
        // Play the game
        err = Play(*server);
        if (err) {
            printf("[server] error while playing game\n");
            break;
        }
    } while(0);

    if (err) __debugbreak();

    //--------------------
    // Cleanup
    server->Stop();
    delete server->yj_server;
    delete server;
    server = {};
    ShutdownYojimbo();

    FreeCommon();
    CloseAudioDevice();
    CloseWindow();

    return err;
}

#include "../common/common.cpp"
#include "game_server.cpp"
#include "editor.cpp"
