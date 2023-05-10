#include "../common/editor.h"
#include "../common/histogram.h"
#include "../common/ui/ui.h"
#include "../common/wang.h"
#include "server_world.h"
#include "game_server.h"
#include "stb_herringbone_wang_tile.h"

void RN_TraceLogCallback(int logLevel, const char *text, va_list args)
{
    const char *outofbounds = "Requested image pixel";
    if (!strncmp(text, "Requested image pixel", strlen(outofbounds))) {
        logLevel = LOG_FATAL;
    }

    if (logLevel > LOG_WARNING) {
        assert(!"raylib is sad");
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

    err = server.world->map.Load(LEVEL_001, server.now);
    if (err) {
        printf("Failed to load map with code %d\n", err);
        assert(!"oops");
        // TODO: Display error message on screen for N seconds or
        // until dismissed
    }

    Tilemap &map = server.world->map;
    Camera2D camera{};
    camera.zoom = 1;

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    const Vector2 uiPosition{ 380, 8 };

    SetExitKey(0);
    bool quit = false;

    WangTileset tileset{};
    err = tileset.GenerateTemplate("resources/wang/template.png");
    if (err) return err;

    tileset.Load(map, "resources/wang/tileset.png");
    WangMap wangMap{};
    err = tileset.GenerateMap(map.width, map.height, map, wangMap);
    if (err) return err;

    Editor editor{};

    while (!quit) {
        io.PushScope(IO::IO_Game);

        server.now = GetTime();
        frameDt = MIN(server.now - frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = server.now;

        server.tickAccum += frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;
        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

        if (io.KeyPressed(KEY_H)) {
            histogram.paused = !histogram.paused;
        }
        if (io.KeyPressed(KEY_V)) {
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
            }
        }

        UpdateCamera(camera);

        while (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            server.Update();
        }

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(BROWN);

        BeginMode2D(camera);

        // [World] Tilemap
        server.world->map.Draw(camera);

        // [Editor] Overlays
        editor.DrawOverlays(map, camera, server.now);

        DrawRectangleLinesEx(lastCollisionA, 1, RED);
        DrawRectangleLinesEx(lastCollisionB, 1, GREEN);

        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (entity.type) {
                entity.Draw(fntHackBold20, entityId, 1);
                if (editor.active && editor.state.showColliders) {
                    // [Debug] Draw colliders
                    if (entity.radius) {
                        DrawCircle(entity.position.x, entity.position.y, entity.radius, entity.colliding ? Fade(RED, 0.5) : Fade(LIME, 0.5));
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
            if (server.world->map.AtTry(0, 0, tile)) {
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
            (float)GetScreenWidth() - screenMargin*2,
            (float)GetScreenHeight() - screenMargin*2,
            }, 1.0f, PINK);
#endif

        editor.DrawUI({ 380, 8 }, map, server.now);

        {
            io.PushScope(IO::IO_HUD);

            const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

            float hud_x = 8.0f;
            float hud_y = 30.0f;
            char buf[128];
#define DRAW_TEXT_MEASURE(measureRect, label, fmt, ...) { \
                if (label) { \
                    snprintf(buf, sizeof(buf), "%-12s : " fmt, label, __VA_ARGS__); \
                } else { \
                    snprintf(buf, sizeof(buf), fmt, __VA_ARGS__); \
                } \
                Vector2 position{ hud_x, hud_y }; \
                DrawTextShadowEx(fntHackBold20, buf, position, RAYWHITE); \
                if (measureRect) { \
                    Vector2 measure = MeasureTextEx(fntHackBold20, buf, (float)fntHackBold20.baseSize, 1.0); \
                    *measureRect = { position.x, position.y, measure.x, measure.y }; \
                } \
                hud_y += fntHackBold20.baseSize; \
            }

#define DRAW_TEXT(label, fmt, ...) \
                DRAW_TEXT_MEASURE((Rectangle *)0, label, fmt, __VA_ARGS__)

            DRAW_TEXT((const char *)0, "%.2f fps (%.2f ms) (vsync=%s)", 1.0 / frameDt, frameDt * 1000.0, IsWindowState(FLAG_VSYNC_HINT) ? "on" : "off");
            DRAW_TEXT("time", "%.02f", server.yj_server->GetTime());
            DRAW_TEXT("tick", "%" PRIu64, server.tick);
            DRAW_TEXT("tickAccum", "%.02f", server.tickAccum);
            DRAW_TEXT("cursor", "%d, %d", GetMouseX(), GetMouseY());
            DRAW_TEXT("cursor world", "%.f, %.f", cursorWorldPos.x, cursorWorldPos.y);
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
                    hud_x += 16.0f;
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
                    hud_x -= 16.0f;
                }
            }

            io.PopScope();
        }

        histogram.Draw({ 8, 8 });

        {
            io.PushScope(IO::IO_EditorUI);

            UIStyle uiWangStyle2x{};
            uiWangStyle2x.scale = 2;
            UIStyle uiWangStyle4x = uiWangStyle2x;
            uiWangStyle4x.scale = 4;
            UI uiWang{ { 8, 180 }, uiWangStyle2x };

            static int hTex = -1;
            static int vTex = -1;

            uiWang.PushStyle(uiWangStyle4x);
                for (int i = 0; i < tileset.ts.num_h_tiles; i++) {
                    stbhw_tile *wangTile = tileset.ts.h_tiles[i];
                    Rectangle srcRect{
                        (float)wangTile->x,
                        (float)wangTile->y,
                        (float)tileset.ts.short_side_len * 2,
                        (float)tileset.ts.short_side_len
                    };
                    if (uiWang.Image(tileset.tsColorized, srcRect).pressed) {
                        hTex = hTex == i ? -1 : i;
                        vTex = -1;
                    }
                }
                uiWang.Newline();

                for (int i = 0; i < tileset.ts.num_v_tiles; i++) {
                    stbhw_tile *wangTile = tileset.ts.v_tiles[i];
                    Rectangle srcRect{
                        (float)wangTile->x,
                        (float)wangTile->y,
                        (float)tileset.ts.short_side_len,
                        (float)tileset.ts.short_side_len * 2
                    };
                    if (uiWang.Image(tileset.tsColorized, srcRect).pressed) {
                        hTex = -1;
                        vTex = vTex == i ? -1 : i;
                    }
                }
                uiWang.Newline();

                if (uiWang.Button("Export tileset").pressed) {
                    ExportImage(tileset.ts.img, "resources/wang/tileset.png");
                }
                uiWang.Newline();
                uiWang.Image(tileset.tsColorized);
                uiWang.Newline();
            uiWang.PopStyle();

            if (uiWang.Button("Re-generate Map").pressed) {
                tileset.GenerateMap(map.width, map.height, map, wangMap);
            }
            uiWang.Newline();
            if (uiWang.Image(wangMap.colorized).pressed) {
                map.SetFromWangMap(wangMap, server.now);
            }

            if (hTex >= 0 || vTex >= 0) {
                Tile selectedTile = editor.state.tiles.cursor.tileDefId;
                static double lastUpdatedAt = 0;
                static double lastChangedAt = 0;
                const double updateDelay = 0.02;

                UIStyle uiWangTileStyle{};
                uiWangTileStyle.margin = 0;
                UI uiWangTile{ { 300, 200 }, uiWangTileStyle };
                if (hTex >= 0) {
                    stbhw_tile *wangTile = tileset.ts.h_tiles[hTex];
                    for (int y = 0; y < tileset.ts.short_side_len; y++) {
                        for (int x = 0; x < tileset.ts.short_side_len*2; x++) {
                            int templateX = wangTile->x + x;
                            int templateY = wangTile->y + y;
                            uint8_t *pixel = &((uint8_t *)tileset.ts.img.data)[(templateY * tileset.ts.img.width + templateX) * 3];
                            uint8_t tile = pixel[0] % map.tileDefCount;
                            const Rectangle tileRect = map.TileDefRect(tile);
                            if (uiWangTile.Image(map.texture, tileRect).down) {
                                pixel[0] = selectedTile; //^ (selectedTile*55);
                                pixel[1] = selectedTile; //^ (selectedTile*55);
                                pixel[2] = selectedTile; //^ (selectedTile*55);
                                lastChangedAt = server.now;
                            }
                        }
                        uiWangTile.Newline();
                    }
                } else if (vTex >= 0) {
                    stbhw_tile *wangTile = tileset.ts.v_tiles[vTex];
                    for (int y = 0; y < tileset.ts.short_side_len*2; y++) {
                        for (int x = 0; x < tileset.ts.short_side_len; x++) {
                            int templateX = wangTile->x + x;
                            int templateY = wangTile->y + y;
                            uint8_t *pixel = &((uint8_t *)tileset.ts.img.data)[(templateY * tileset.ts.img.width + templateX) * 3];
                            uint8_t tile = pixel[0] % map.tileDefCount;
                            const Rectangle tileRect = map.TileDefRect(tile);
                            if (uiWangTile.Image(map.texture, tileRect).down) {
                                pixel[0] = selectedTile; //^ (selectedTile*55);
                                pixel[1] = selectedTile; //^ (selectedTile*55);
                                pixel[2] = selectedTile; //^ (selectedTile*55);
                                lastChangedAt = server.now;
                            }
                        }
                        uiWangTile.Newline();
                    }
                }

                // Update if dirty on a slight delay (to make dragging more efficient)
                if (lastChangedAt > lastUpdatedAt && (server.now - lastChangedAt) > updateDelay) {
                    UnloadTexture(tileset.tsColorized);
                    tileset.tsColorized = tileset.GenerateColorizedTexture(tileset.ts.img, map);
                    lastUpdatedAt = server.now;
                }
            }

            io.PopScope();
        }

        EndDrawing();
        //yojimbo_sleep(0.001);

        // Nobody else handled it, so user probably wants to quit
        if (WindowShouldClose() || io.KeyPressed(KEY_ESCAPE)) {
            quit = true;
        }

        io.PopScope();
        io.EndFrame();
    }

    tileset.Unload();
    return err;
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    Err err = RN_SUCCESS;
    //SetTraceLogLevel(LOG_WARNING);
    SetTraceLogCallback(RN_TraceLogCallback);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
    //SetWindowState(FLAG_VSYNC_HINT);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

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
    // Create world
    server->world = new ServerWorld;
    if (!server->world) {
        printf("error: failed to allocate world\n");
        return RN_BAD_ALLOC;
    }

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