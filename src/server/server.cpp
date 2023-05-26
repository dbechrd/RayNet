#include "../common/collision.h"
#include "../common/editor.h"
#include "../common/histogram.h"
#include "../common/ui/ui.h"
#include "game_server.h"

void RN_TraceLogCallback(int logLevel, const char *text, va_list args)
{
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
}

Err Play(GameServer &server)
{
    Err err = RN_SUCCESS;

    Tilemap &map = server.map;
    err = map.Load(LEVEL_001, server.now);
    if (err) {
        printf("Failed to load map with code %d\n", err);
        assert(!"oops");
        // TODO: Display error message on screen for N seconds or
        // until dismissed
    }

    Camera2D camera{};
    camera.zoom = 1;

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    const Vector2 uiPosition{ 380, 8 };

    SetExitKey(0);
    bool quit = false;

    Editor editor{};
    editor.Init(map);

    while (!quit) {
        io.PushScope(IO::IO_Game);

        server.now = GetTime();
        frameDt = MIN(server.now - frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = server.now;

        server.tickAccum += frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;
        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

        // Input
        if (io.KeyPressed(KEY_F11)) {
            bool isFullScreen = IsWindowState(FLAG_FULLSCREEN_MODE);
            if (isFullScreen) {
                ClearWindowState(FLAG_FULLSCREEN_MODE);
            } else {
                SetWindowState(FLAG_FULLSCREEN_MODE);
            }
        }
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
            histogram.paused = !histogram.paused;
        }

        UpdateCamera(camera);

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
        map.Draw(camera);

        // [Editor] Overlays
        editor.DrawOverlays(map, camera, server.now);

        DrawRectangleLinesEx(lastCollisionA, 1, RED);
        DrawRectangleLinesEx(lastCollisionB, 1, GREEN);

        // NOTE(dlb): We could build an array of { entityId, position.y } and sort it
        // each frame, then render the entities in that order.
        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = map.entities[entityId];
            if (entity.type) {
                map.DrawEntity(entityId, server.now);
                if (editor.active && editor.state.showColliders) {
                    // [Debug] Draw colliders
                    AspectCollision &collider = map.collision[entityId];
                    if (collider.radius) {
                        DrawCircle(entity.position.x, entity.position.y, collider.radius, collider.colliding ? Fade(RED, 0.5) : Fade(LIME, 0.5));
                    }
                }

                // [Debug] Draw velocity vectors
                //DrawLineEx(entity.position, Vector2Add(entity.position, entity.velocity), 2, PINK);

#if CL_DBG_FORCE_ACCUM
                // [Debug] Draw force vectors
                if (Vector2LengthSqr(entity.forceAccum)) {
                    DrawLineEx(
                        { entity.position.x, entity.position.y },
                        { entity.position.x + entity.forceAccum.x, entity.position.y + entity.forceAccum.y },
                        2,
                        YELLOW
                    );
                }
#endif
            }
        }

#if CL_DBG_CIRCLE_VS_REC
        {
            const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
            Tile tile{};
            if (map.AtTry(0, 0, tile)) {
                Rectangle tileRect{};
                Vector2 tilePos = { 0, 0 };
                tileRect.x = tilePos.x;
                tileRect.y = tilePos.y;
                tileRect.width = TILE_W;
                tileRect.height = TILE_W;
                Manifold manifold{};
                if (dlb_CheckCollisionCircleRec(cursorWorldPos, 10, tileRect, &manifold)) {
                    DrawCircle(cursorWorldPos.x, cursorWorldPos.y, 10, Fade(RED, 0.5f));

                    Vector2 resolve = Vector2Scale(manifold.normal, manifold.depth);
                    DrawLine(
                        manifold.contact.x,
                        manifold.contact.y,
                        manifold.contact.x + resolve.x,
                        manifold.contact.y + resolve.y,
                        ORANGE
                    );
                    DrawCircle(manifold.contact.x, manifold.contact.y, 1, BLUE);
                    DrawCircle(manifold.contact.x + resolve.x, manifold.contact.y + resolve.y, 1, ORANGE);

                    DrawCircle(cursorWorldPos.x + resolve.x, cursorWorldPos.y + resolve.y, 10, Fade(LIME, 0.5f));
                } else {
                    DrawCircle(cursorWorldPos.x, cursorWorldPos.y, 10, Fade(GRAY, 0.5f));
                }
            }
        }
#endif

        EndMode2D();

#if CL_DBG_TILE_CULLING
        // Screen bounds debug rect for tile culling
        DrawRectangleLinesEx({
            screenMargin,
            screenMargin,
            (float)GetRenderWidth() - screenMargin*2,
            (float)GetRenderHeight() - screenMargin*2,
            }, 1.0f, PINK);
#endif

        editor.DrawUI({}, map, server.now);

        {
            io.PushScope(IO::IO_F3Menu);

            Vector2 hudCursor{
                GetRenderWidth() - 360.0f - 8.0f,
                8.0f
            };

            histogram.Draw(hudCursor);
            hudCursor.y += histogram.histoHeight + 8;

            char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                if (label) { \
                    snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                } else { \
                    snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
                } \
                DrawTextShadowEx(fntHackBold20, buf, hudCursor, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntHackBold20, buf, (float)fntHackBold20.baseSize, 1.0); \
                    *measureRect = { hudCursor.x,hudCursor.y, measure.x, measure.y }; \
                } \
                hudCursor.y += fntHackBold20.baseSize; \
            }

#define DRAW_TEXT(label, fmt, ...) \
            DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
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

            io.PopScope();
        }

        EndDrawing();
        yojimbo_sleep(0.001);

        // Nobody else handled it, so user probably wants to quit
        if (WindowShouldClose() || io.KeyPressed(KEY_ESCAPE)) {
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
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogCallback(RN_TraceLogCallback);

    InitWindow(1920, 1017, "RayNet Server");
    //SetWindowState(FLAG_VSYNC_HINT);  // KEEP THIS ENABLED it makes the room cooler
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_WINDOW_MAXIMIZED);

    InitAudioDevice();

    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
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

    //--------------------
    // Create server
    // NOTE(DLB): MUST happen after InitWindow() so that GetTime() is valid!!
    GameServer *server = new GameServer;
    if (!server) {
        printf("error: failed to allocate server\n");
        return RN_BAD_ALLOC;
    }
    server->now = GetTime();

    //--------------------
    // Start the server
    err = server->Start();
    if (err) {
        printf("error: failed to start server\n");
        return err;
    }

    //--------------------
    // Play the game
    err = Play(*server);
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
