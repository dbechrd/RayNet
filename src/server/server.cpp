#include "../common/boot_screen.h"
#include "../common/collision.h"
#include "../common/histogram.h"
#include "../common/io.h"
#include "../common/perf_timer.h"
#include "../common/ui/ui.h"
#include "editor.h"
#include "f3_menu.h"
#include "game_server.h"

const bool IS_SERVER = true;

void RN_TraceLogCallback(int logLevel, const char *text, va_list args)
{
    const char *outofbounds = "Requested image pixel";
    if (!strncmp(text, outofbounds, strlen(outofbounds))) {
        logLevel = LOG_FATAL;
    }

    // Message has level below current threshold, don't emit
    //if (logType < TraceLogLevel) return;

    char buffer[128] = { 0 };

    switch (logLevel)
    {
        case LOG_TRACE:   strcpy(buffer, "TRACE:   "); break;
        case LOG_DEBUG:   strcpy(buffer, "DEBUG:   "); break;
        case LOG_INFO:    strcpy(buffer, "INFO:    "); break;
        case LOG_WARNING: strcpy(buffer, "WARNING: "); break;
        case LOG_ERROR:   strcpy(buffer, "ERROR:   "); break;
        case LOG_FATAL:   strcpy(buffer, "FATAL:   "); break;
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

Err Play(GameServer &server)
{
    Err err = RN_SUCCESS;

    Tilemap &level_001 = pack_maps.FindByName<Tilemap>(MAP_OVERWORLD);
    
    Camera2D camera{};
    camera.zoom = 1;
    camera.target = { level_001.width * TILE_W / 2.0f, level_001.height * TILE_W / 2.0f };
    camera.offset = { GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };

    // fountain, for anya testing
    camera.target = { 1630, 3060 };
    camera.offset = Vector2Floor(camera.offset);

    Editor editor{ level_001.id };
    editor.Init();

    bool quit = false;
    while (!quit) {
        g_RenderSize.x = GetRenderWidth();
        g_RenderSize.y = GetRenderHeight();

        io.PushScope(IO::IO_Game);

        server.frame++;
        server.now = GetTime();
        server.frameDt = MIN(server.now - server.frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        server.frameDtSmooth = LERP(server.frameDtSmooth, server.frameDt, 0.1);
        server.frameStart = server.now;
        server.tickAccum += server.frameDt;

#if SV_RENDER
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

        editor.HandleInput(camera, server.now, server.frameDt);

        UpdateCamera(camera);

        Histogram::Entry histoEntry{ server.frame, server.now };
        histoEntry.value = server.frameDt * 1000.0f;
        const bool doNetTick = server.tickAccum >= SV_TICK_DT;
        histoEntry.color = doNetTick ? DARKPURPLE : RAYWHITE;
        histoFps.Push(histoEntry);
#endif

        uint64_t oldTick = server.tick;
        if (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            server.Update();
        }
        const double dt = (server.tick - oldTick) * SV_TICK_DT;

#if SV_RENDER
        if (dt) {
            // Editor/graphical stuff
            UpdateTileDefAnimations(dt);
        }

        //--------------------
        // Draw
        BeginDrawing();
            ClearBackground(BLUE_DESAT);
            BeginMode2D(camera);
                auto &editor_map = pack_maps.FindById<Tilemap>(editor.map_id);

                DrawCmdQueue sortedDraws{};

                // [World] Draw ground tiles
                editor_map.Draw(camera, sortedDraws);

                for (Entity &entity : entityDb->entities) {
                    if (entity.map_id == editor_map.id) {
                        entityDb->DrawEntity(entity, sortedDraws);
                    }
                }

                // [World] Draw sorted object tiles and entities
                sortedDraws.Draw();

                // [Editor] Overlays
                editor.DrawGroundOverlays(camera);

                // [Debug] Last collision
                DrawRectangleLinesEx(server.lastCollisionA, 1, RED);
                DrawRectangleLinesEx(server.lastCollisionB, 1, GREEN);
            EndMode2D();

            // [Editor] Overlays
            editor.DrawEntityOverlays(camera);

            // [Editor] Menus, action bar, etc.
            editor.DrawUI(camera);

            assert(scissorStack.empty());

            // [Debug] FPS, clock, etc.
            if (server.showF3Menu) {
                Vector2 f3_pos{
                    editor.dock_left ? g_RenderSize.x - 360.0f - 8.0f : 8.0f,
                    8.0f
                };
                F3Menu_Draw(server, camera, f3_pos);
            }
        EndDrawing();  // has to be last thing in loop (i.e. raylib does all its finalization stuff here)
#else
        BeginDrawing();
        EndDrawing();
#endif
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
    PerfTimer t{ "Server" };

    GameServer *server{};

    do {
        SetTraceLogLevel(LOG_WARNING);
        SetTraceLogCallback(RN_TraceLogCallback);

        {
            PerfTimer t{ "InitWindow" };
            InitWindow(1920, 1017, "RayNet Server");
            SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
            SetWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowState(FLAG_WINDOW_MAXIMIZED);
            SetWindowState(FLAG_VSYNC_HINT);  // KEEP THIS ENABLED it makes the room cooler
            SetExitKey(0);  // must be called after InitWindow()
            g_RenderSize.x = GetRenderWidth();
            g_RenderSize.y = GetRenderHeight();

            DrawBootScreen();
        }

        {
            PerfTimer t{ "InitAudioDevice" };
            InitAudioDevice();
            SetMasterVolume(0);
        }

        // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
        srand((unsigned int)GetTime());

        err = Init();
        if (err) {
            printf("Failed to load common resources\n");
            break;
        }

        Image icon = LoadImage("../res/server.png");
        SetWindowIcon(icon);
        UnloadImage(icon);

        Vector2 screenSize = { g_RenderSize.x, g_RenderSize.y };

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
        //if (!server->FindOrLoadMap(MAP_OVERWORLD)) {
        //    err = RN_BAD_FILE_READ;
        //    break;
        //}
        //if (!server->FindOrLoadMap(MAP_CAVE)) break;

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

    Free();
    CloseAudioDevice();
    CloseWindow();

    return err;
}

#include "../common/common.cpp"
#include "../common/boot_screen.cpp"
#include "editor.cpp"
#include "f3_menu.cpp"
#include "game_server.cpp"