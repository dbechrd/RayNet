#include "../common/shared_lib.h"
#include "../common/histogram.h"
#include "../common/ui/ui.h"
#include "server_world.h"
#include "game_server.h"
#include "stb_herringbone_wang_tile.h"
#include <stack>
#include <cassert>

struct Cursor {
    int tileDefId;
} cursor{};

bool NeedsFill(Tilemap &map, int x, int y, int tileDefFill)
{
    Tile tile;
    if (map.AtTry(x, y, tile)) {
        return tile == tileDefFill;
    }
    return false;
}

void Scan(Tilemap &map, int lx, int rx, int y, int tileDefFill, std::stack<Tilemap::Coord> &stack)
{
    bool inSpan = false;
    for (int x = lx; x < rx; x++) {
        if (!NeedsFill(map, x, y, tileDefFill)) {
            inSpan = false;
        } else if (!inSpan) {
            stack.push({ x, y });
            inSpan = true;
        }
    }
}

void Fill(Tilemap &map, int x, int y, int tileDefId)
{
    int tileDefFill = map.At(x, y);
    if (tileDefFill == tileDefId) {
        return;
    }

    std::stack<Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Tilemap::Coord coord = stack.top();
        stack.pop();

        int lx = coord.x;
        int rx = coord.x;
        while (NeedsFill(map, lx - 1, coord.y, tileDefFill)) {
            map.Set(lx - 1, coord.y, tileDefId);
            lx -= 1;
        }
        while (NeedsFill(map, rx, coord.y, tileDefFill)) {
            map.Set(rx, coord.y, tileDefId);
            rx += 1;
        }
        Scan(map, lx, rx, coord.y - 1, tileDefFill, stack);
        Scan(map, lx, rx, coord.y + 1, tileDefFill, stack);
    }
}

Err Play(GameServer &server)
{
    Err err = RN_SUCCESS;

    err = InitCommon();
    if (err) {
        printf("Failed to load common resources\n");
    }

    err = server.world->map.Load(LEVEL_001);
    if (err) {
        printf("Failed to load map with code %d\n", err);
        assert(!"oops");
        // TODO: Display error message on screen for N seconds or
        // until dismissed
    }

    Camera2D &camera2d = server.world->camera2d;
    Tilemap &map = server.world->map;

    /*Camera3D camera3d{};
    camera3d.position = {0, 0, 0};
    camera3d.target = {0, 0, -1};
    camera3d.up = {0, 1, 0};
    camera3d.fovy = 90;
    camera3d.projection = CAMERA_PERSPECTIVE;
    SetCameraMode(camera3d, CAMERA_FREE);*/

    camera2d.zoom = 1.0f;

    Histogram histogram{};
    double frameStart = GetTime();
    double frameDt = 0;
    double frameDtSmooth = 60;

    bool editorActive = false;
    const Vector2 uiPosition{ 380, 8 };
    const Rectangle editorRect{ uiPosition.x, uiPosition.y, 800, 80 };

    SetExitKey(0);

    Texture wangTemplateTex{};
    stbhw_tileset tileset{};
    Texture wangHTileTex{};
    Texture wangVTileTex{};
    Texture wangMapTex{};

    while (!WindowShouldClose())
    {
        server.now = GetTime();
        frameDt = MIN(server.now - frameStart, SV_TICK_DT * 3);  // arbitrary limit for now
        frameDtSmooth = LERP(frameDtSmooth, frameDt, 0.1);
        frameStart = server.now;

        server.tickAccum += frameDt;

        bool doNetTick = server.tickAccum >= SV_TICK_DT;

        bool escape = IsKeyPressed(KEY_ESCAPE);

        if (IsKeyPressed(KEY_H)) {
            histogram.paused = !histogram.paused;
        }
        if (IsKeyPressed(KEY_V)) {
            if (IsWindowState(FLAG_VSYNC_HINT)) {
                ClearWindowState(FLAG_VSYNC_HINT);
            } else {
                SetWindowState(FLAG_VSYNC_HINT);
            }
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ZERO)) {
            camera2d.zoom = 1.0f;
        }
        if (IsKeyPressed(KEY_GRAVE)) {
            editorActive = !editorActive;
        }

        histogram.Push(frameDtSmooth, doNetTick ? GREEN : RAYWHITE);

        if (editorActive) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f / camera2d.zoom);

                camera2d.target = Vector2Add(camera2d.target, delta);
            }

            // Zoom based on mouse wheel
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                // Get the world point that is under the mouse
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera2d);

                // Set the offset to where the mouse is
                camera2d.offset = GetMousePosition();

                // Set the target to match, so that the camera maps the world space point
                // under the cursor to the screen space point under the cursor at any zoom
                camera2d.target = mouseWorldPos;

                // Zoom increment
                const float zoomIncrement = 0.125f;

                camera2d.zoom += (wheel * zoomIncrement * camera2d.zoom);
                if (camera2d.zoom < zoomIncrement) camera2d.zoom = zoomIncrement;
            }
        } else {
            //camera2d.offset = {};
            //camera2d.target = {};
            //camera2d.zoom = 1;
        }

        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera2d);

        while (server.tickAccum >= SV_TICK_DT) {
            //printf("[%.2f][%.2f] ServerUpdate %d\n", server.tickAccum, now, (int)server.tick);
            server.Update();
        }

        //--------------------
        // Draw
        BeginDrawing();
        ClearBackground(BROWN);

        BeginMode2D(camera2d);

        // [World] Tilemap
        server.world->map.Draw(camera2d);

        DrawRectangleLinesEx(lastCollisionA, 1, RED);
        DrawRectangleLinesEx(lastCollisionB, 1, GREEN);

        // [World][Editor] Paths

        // Draw path edges
        for (uint32_t aiPathId = 0; aiPathId < server.world->map.pathCount; aiPathId++) {
            AiPath *aiPath = server.world->map.GetPath(aiPathId);
            if (!aiPath) {
                continue;
            }

            for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
                uint32_t aiPathNodeNextIndex = server.world->map.GetNextPathNodeIndex(aiPathId, aiPathNodeIndex);
                AiPathNode *aiPathNode = server.world->map.GetPathNode(aiPathId, aiPathNodeIndex);
                AiPathNode *aiPathNodeNext = server.world->map.GetPathNode(aiPathId, aiPathNodeNextIndex);
                assert(aiPathNode);
                assert(aiPathNodeNext);
                DrawLine(
                    aiPathNode->pos.x, aiPathNode->pos.y,
                    aiPathNodeNext->pos.x, aiPathNodeNext->pos.y,
                    LIGHTGRAY
                );
            }
        }

        static struct PathNodeDrag {
            bool active;
            uint32_t pathId;
            uint32_t pathNodeIndex;
            Vector2 startPosition;
        } aiPathNodeDrag{};

        // Draw path nodes
        const float pathRectRadius = 5;
        for (uint32_t aiPathId = 0; aiPathId < server.world->map.pathCount; aiPathId++) {
            AiPath *aiPath = server.world->map.GetPath(aiPathId);
            if (!aiPath) {
                continue;
            }

            for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
                AiPathNode *aiPathNode = server.world->map.GetPathNode(aiPathId, aiPathNodeIndex);
                assert(aiPathNode);

                Rectangle nodeRect{
                    aiPathNode->pos.x - pathRectRadius,
                    aiPathNode->pos.y - pathRectRadius,
                    pathRectRadius * 2,
                    pathRectRadius * 2
                };

                Color color = aiPathNode->waitFor ? BLUE : RED;
                bool hover = dlb_CheckCollisionPointRec(cursorWorldPos, nodeRect);
                if (hover || (aiPathNodeDrag.active && aiPathNodeDrag.pathNodeIndex == aiPathNodeIndex)) {
                    if (hover) {
                        color = aiPathNode->waitFor ? SKYBLUE : PINK;
                    }
                    bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                    if (down) {
                        color = aiPathNode->waitFor ? DARKBLUE : MAROON;
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            aiPathNodeDrag.active = true;
                            aiPathNodeDrag.pathId = aiPathId;
                            aiPathNodeDrag.pathNodeIndex = aiPathNodeIndex;
                            aiPathNodeDrag.startPosition = aiPathNode->pos;
                        } else if (escape) {
                            aiPathNode->pos = aiPathNodeDrag.startPosition;
                            aiPathNodeDrag = {};
                            escape = false;
                        }
                    } else {
                        aiPathNodeDrag = {};
                    }
                }
                DrawRectangleRec(nodeRect, color);
            }
        }

        // NOTE(dlb): I could rewrite the loop above again below to have draw happen
        // after drag update.. but meh. I don't wanna duplicate code, so dragging nodes
        // will have 1 frame of delay. :(
        if (aiPathNodeDrag.active) {
            Vector2 newNodePos = cursorWorldPos;
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                newNodePos.x = roundf(newNodePos.x / 8) * 8;
                newNodePos.y = roundf(newNodePos.y / 8) * 8;
            }
            Vector2SubtractValue(newNodePos, pathRectRadius);
            AiPath *aiPath = server.world->map.GetPath(aiPathNodeDrag.pathId);
            if (aiPath) {
                AiPathNode *aiPathNode = server.world->map.GetPathNode(aiPathNodeDrag.pathId, aiPathNodeDrag.pathNodeIndex);
                assert(aiPathNode);
                aiPathNode->pos = newNodePos;
            }
        }

        const bool editorHovered = dlb_CheckCollisionPointRec(GetMousePosition(), editorRect);

        // [Editor] Tile actions and hover highlight
        if (editorActive && !editorHovered) {
            const bool editorPlaceTile = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            const bool editorPickTile = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
            const bool editorFillTile = IsKeyPressed(KEY_F);

            Tile hoveredTile;
            bool hoveringTile = map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile);
            if (hoveringTile) {
                Tilemap::Coord coord{};
                bool validCoord = map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
                assert(validCoord);  // should always be true when hoveredTile != null

                if (editorPlaceTile) {
                    map.Set(coord.x, coord.y, cursor.tileDefId);
                } else if (editorPickTile) {
                    cursor.tileDefId = hoveredTile;
                } else if (editorFillTile) {
                    Fill(map, coord.x, coord.y, cursor.tileDefId);
                }

                DrawRectangleLinesEx({ (float)coord.x * TILE_W, (float)coord.y * TILE_W, TILE_W, TILE_W }, 2, WHITE);
            }
        }

        for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
            Entity &entity = server.world->entities[entityId];
            if (entity.type) {
                entity.Draw(fntHackBold20, entityId, 1);
#if CL_DBG_COLLIDERS
                // [Debug] Draw colliders
                if (entity.radius) {
                    DrawCircle(entity.position.x, entity.position.y, entity.radius, entity.colliding ? Fade(RED, 0.5) : Fade(GRAY, 0.5));
                }
#endif
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
            Tile tile{};
            if (server.world->map.AtTry(0, 0, tile)) {
                Rectangle tileRect{};
                Vector2 tilePos = { 0, 0 };
                tileRect.x = tilePos.x;
                tileRect.y = tilePos.y;
                tileRect.width = TILE_W;
                tileRect.height = TILE_W;
                Vector2 contact{};
                Vector2 normal{};
                float depth{};
                if (dlb_CheckCollisionCircleRec(cursorWorldPos, 10, tileRect, contact, normal, depth)) {
                    DrawCircle(cursorWorldPos.x, cursorWorldPos.y, 10, Fade(RED, 0.5f));

                    Vector2 resolve = Vector2Scale(normal, depth);
                    DrawLine(
                        contact.x,
                        contact.y,
                        contact.x + resolve.x,
                        contact.y + resolve.y,
                        ORANGE
                    );
                    DrawCircle(contact.x, contact.y, 1, BLUE);
                    DrawCircle(contact.x + resolve.x, contact.y + resolve.y, 1, ORANGE);

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

        // [Editor] Action Bar
        if (editorActive) {
            UIStyle uiEditorMenuStyle{};
            UI uiEditorMenu{ uiPosition, uiEditorMenuStyle };

            // TODO: UI::Panel
            DrawRectangleRounded(editorRect, 0.2f, 6, ASESPRITE_BEIGE);
            DrawRectangleRoundedLines(editorRect, 0.2f, 6, 2.0f, BLACK);

            UIState saveButton = uiEditorMenu.Button("Save");
            if (saveButton.clicked) {
                if (map.Save(LEVEL_001) != RN_SUCCESS) {
                    // TODO: Display error message on screen for N seconds or
                    // until dismissed
                }
            }

            UIState loadButton = uiEditorMenu.Button("Load");
            if (loadButton.clicked) {
                Err err = map.Load(LEVEL_001);
                if (err != RN_SUCCESS) {
                    printf("Failed to load map with code %d\n", err);
                    assert(!"oops");
                    // TODO: Display error message on screen for N seconds or
                    // until dismissed
                }
            }

            uiEditorMenu.Newline();

            // [Editor] Tile selector
            const bool editorPickTileDef = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            // TODO(dlb): UI::Panel (or row.style.colorBg?)
            Vector2 uiCursor = uiEditorMenu.CursorScreen();
            uiCursor = Vector2Add(uiCursor, uiEditorMenuStyle.margin.TopLeft());

            DrawRectangle(
                uiCursor.x,
                uiCursor.y,
                server.world->map.tileDefCount * TILE_W + server.world->map.tileDefCount * 2 + 2,
                TILE_W + 4,
                BLACK
            );

            for (uint32_t i = 0; i < server.world->map.tileDefCount; i++) {
                TileDef &tileDef = server.world->map.tileDefs[i];
                Rectangle texRect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
                Vector2 screenPos = {
                    uiCursor.x + 2 + i * TILE_W + i * 2,
                    uiCursor.y + 2
                };

                Rectangle tileDefRectScreen{ screenPos.x, screenPos.y, TILE_W, TILE_W };
                bool hover = dlb_CheckCollisionPointRec(GetMousePosition(), tileDefRectScreen);
                if (hover) {
                    static int prevTileDefHovered = -1;
                    if (editorPickTileDef) {
                        PlaySound(sndHardTick);
                        cursor.tileDefId = i;
                    } else if (i != prevTileDefHovered) {
                        PlaySound(sndSoftTick);
                    }
                    prevTileDefHovered = i;
                }

                DrawTextureRec(server.world->map.texture, texRect, screenPos, WHITE);
                const int outlinePad = 1;
                if (i == cursor.tileDefId || hover) {
                    DrawRectangleLinesEx(
                        {
                            screenPos.x - outlinePad,
                            screenPos.y - outlinePad,
                            TILE_W + outlinePad * 2,
                            TILE_W + outlinePad * 2
                        },
                        2,
                        i == cursor.tileDefId ? YELLOW : WHITE
                    );
                }
            }
        }

        {
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
            DRAW_TEXT("cursor", "%d, %d (world: %.f, %.f)", GetMouseX(), GetMouseY(), cursorWorldPos.x, cursorWorldPos.y);
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
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                    && CheckCollisionPointRec({ (float)GetMouseX(), (float)GetMouseY() }, clientRowRect))
                {
                    showClientInfo[clientIdx] = !showClientInfo[clientIdx];
                }
                if (showClientInfo[clientIdx]) {
                    hud_x += 16.0f;
                    yojimbo::NetworkInfo netInfo{};
                    server.yj_server->GetNetworkInfo(clientIdx, netInfo);
                    DRAW_TEXT("  rtt", "%f.02", netInfo.RTT);
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
        }

        histogram.Draw({ 8, 8 });

        if (!wangTemplateTex.width) {
            stbhw_config config{};
            config.is_corner = 0;
            config.short_side_len = 8;
            config.num_color[0] = 1;
            config.num_color[1] = 1;
            config.num_color[2] = 1;
            config.num_color[3] = 1;
            config.num_color[4] = 1;
            config.num_color[5] = 1;
            config.num_vary_x = 1;
            config.num_vary_y = 1;

            int wangTemplateW{};
            int wangTemplateH{};
            stbhw_get_template_size(&config, &wangTemplateW, &wangTemplateH);
            unsigned char *wangTemplatePixels = (unsigned char *)malloc((size_t)3 * wangTemplateW * wangTemplateH);
            if (!stbhw_make_template(&config, wangTemplatePixels, wangTemplateW, wangTemplateH, wangTemplateW*3)) {
                printf("[stbhw] failed to generate tileset template. %s\n", stbhw_get_last_error());
                return RN_BAD_ALLOC;
            }

            //if (!stbhw_generate_image(&ts, NULL, data, xs*3, xs, ys)) {
            //    printf("stbhw error: %s\n", stbhw_get_last_error());
            //    return RN_BAD_ALLOC;
            //}

            Image wangTemplateImg{};
            wangTemplateImg.width = wangTemplateW;
            wangTemplateImg.height = wangTemplateH;
            wangTemplateImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
            wangTemplateImg.data = wangTemplatePixels;
            wangTemplateImg.mipmaps = 1;
            wangTemplateTex = LoadTextureFromImage(wangTemplateImg);
            bool exportSuccess = ExportImage(wangTemplateImg, "resources/wang/template.png");
            free(wangTemplatePixels);

            //stbi_write_png("test_map.png", xs, ys, 3, data, xs*3);

            //stbhw_free_tileset(&ts);
        }

        if (!wangHTileTex.width) {
            const char *wangTilesetPath = "resources/wang/tileset.png";
            Image wangTilesetImg = LoadImage(wangTilesetPath);
            if (wangTilesetImg.width) {
                ImageFormat(&wangTilesetImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);

                if (!stbhw_build_tileset_from_image(&tileset, (unsigned char *)wangTilesetImg.data, wangTilesetImg.width*3, wangTilesetImg.width, wangTilesetImg.height)) {
                    printf("[stbhw] failed to build tileset from image %s. %s\n", wangTilesetPath, stbhw_get_last_error());
                    return RN_BAD_ALLOC;
                }

                UnloadImage(wangTilesetImg);

                stbhw_tile *h_tile = tileset.h_tiles[0];
                stbhw_tile *v_tile = tileset.v_tiles[0];

                Image wangHTile{};
                wangHTile.data = h_tile->pixels;
                wangHTile.width = tileset.short_side_len * 2;
                wangHTile.height = tileset.short_side_len;
                wangHTile.mipmaps = 1;
                wangHTile.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
                wangHTileTex = LoadTextureFromImage(wangHTile);

                Image wangVTile{};
                wangVTile.data = v_tile->pixels;
                wangVTile.width = tileset.short_side_len;
                wangVTile.height = tileset.short_side_len * 2;
                wangVTile.mipmaps = 1;
                wangVTile.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
                wangVTileTex = LoadTextureFromImage(wangVTile);
            }
        }

        if (!wangMapTex.width) {
            int wangMapW = map.width;
            int wangMapH = map.height;
            unsigned char *wangMapPixels = (unsigned char *)malloc((size_t)3 * wangMapW * wangMapH);
            if (!stbhw_generate_image(&tileset, 0, wangMapPixels, wangMapW*3, wangMapW, wangMapH)) {
                printf("[stbhw] failed to generate map from tileset. %s\n", stbhw_get_last_error());
                return RN_BAD_ALLOC;
            }

            Image wangMapImg{};
            wangMapImg.width = wangMapW;
            wangMapImg.height = wangMapH;
            wangMapImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
            wangMapImg.data = wangMapPixels;
            wangMapImg.mipmaps = 1;
            ImageFormat(&wangMapImg, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
            wangMapTex = LoadTextureFromImage(wangMapImg);

            for (int y = 0; y < map.height; y++) {
                for (int x = 0; x < map.width; x++) {
                    uint8_t pixel = ((uint8_t *)wangMapImg.data)[y * map.width + x];
                    int tileType = (int)floorf(((float)pixel / 256) * map.tileDefCount);
                    tileType = CLAMP(tileType, 0, map.tileDefCount - 1);
                    //map.Set(x, y, tileType);
                }
            }

            UnloadImage(wangMapImg);
            //free(wangMapPixels);
        }

        // TODO ui.Image();
        const float wangScale = 4;
        Vector2 wangCursor{ 8, 300 };
        DrawTextureEx(wangTemplateTex, wangCursor, 0, wangScale, WHITE);
        wangCursor.y += wangTemplateTex.height * wangScale + 10;
        DrawTextureEx(wangHTileTex, wangCursor, 0, wangScale, WHITE);
        wangCursor.y += wangHTileTex.height * wangScale + 10;
        DrawTextureEx(wangVTileTex, wangCursor, 0, wangScale, WHITE);
        wangCursor.y += wangVTileTex.height * wangScale + 10;
        DrawTextureEx(wangMapTex, wangCursor, 0, wangScale, WHITE);

        EndDrawing();
        //yojimbo_sleep(0.001);

        // Nobody else handled it, so user probably wants to quit
        if (escape) {
            CloseWindow();
        }
    }

    stbhw_free_tileset(&tileset);
    UnloadTexture(wangTemplateTex);
    UnloadTexture(wangHTileTex);
    UnloadTexture(wangVTileTex);

    FreeCommon();
    return err;
}

int main(int argc, char *argv[])
//int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *pCmdLine, int nCmdShow)
{
    Err err = RN_SUCCESS;
    //SetTraceLogLevel(LOG_WARNING);

    InitAudioDevice();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RayNet Server");
    // NOTE(dlb): yojimbo uses rand() for network simulator and random_int()/random_float()
    srand((unsigned int)GetTime());

    //SetWindowState(FLAG_VSYNC_HINT);
    //SetWindowState(FLAG_WINDOW_RESIZABLE);

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

    //--------------------
    // Cleanup
    server->Stop();
    delete server->yj_server;
    delete server;
    server = {};
    ShutdownYojimbo();
    CloseAudioDevice();

    return err;
}