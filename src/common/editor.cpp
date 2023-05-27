#include "audio/audio.h"
#include "collision.h"
#include "editor.h"
#include "stb_herringbone_wang_tile.h"
#include "texture_catalog.h"
#include "tilemap.h"
#include "tinyfiledialogs.h"
#include "ui/ui.h"

const char *EditModeStr(EditMode mode)
{
    switch (mode) {
        case EditMode_Tiles:    return "Tiles";
        case EditMode_Wang:     return "Wang";
        case EditMode_Paths:    return "Paths";
        case EditMode_Warps:    return "Warps";
        case EditMode_Entities: return "Entities";
        default: return "<null>";
    }
}

Err Editor::Init(Tilemap &map)
{
    Err err = RN_SUCCESS;
    err = state.wang.wangTileset.Load(map, "resources/wang/tileset2x2.png");
    return err;
}

void Editor::HandleInput(Camera2D &camera)
{
    io.PushScope(IO::IO_Editor);

    if (io.KeyPressed(KEY_GRAVE)) {
        active = !active;
    }
    if (io.KeyPressed(KEY_ESCAPE)) {
        active = false;
    }
    if (io.KeyDown(KEY_LEFT_CONTROL) && io.KeyPressed(KEY_ZERO)) {
        camera.zoom = 1.0f;
    }

    if (active) {
        io.CaptureKeyboard();
    }

    io.PopScope();
}

void Editor::DrawGroundOverlays(Tilemap &map, Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorGroundOverlay);

    // Draw tile collision layer
    if (state.showColliders) {
        map.DrawColliders(camera);
    }
    if (state.showTileIds) {
        map.DrawTileIds(camera);
    }

    if (active) {
        switch (mode) {
            case EditMode_Tiles: {
                DrawGroundOverlay_Tiles(map, camera, now);
                break;
            }
            case EditMode_Wang: {
                DrawGroundOverlay_Wang(map, camera, now);
                break;
            }
            case EditMode_Paths: {
                DrawGroundOverlay_Paths(map, camera, now);
                break;
            }
            case EditMode_Warps: {
                DrawGroundOverlay_Warps(map, camera, now);
                break;
            }
            case EditMode_Entities: {
                break;
            }
        }
    }

    io.PopScope();
}
void Editor::DrawGroundOverlay_Tiles(Tilemap &map, Camera2D &camera, double now)
{
    if (!io.MouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPlaceTile = io.MouseButtonDown(MOUSE_BUTTON_LEFT);
        const bool editorPickTile = io.MouseButtonDown(MOUSE_BUTTON_MIDDLE);
        const bool editorFillTile = io.KeyPressed(KEY_F);

        Tile &cursorTile = state.tiles.cursor.tileDefId;
        Tile hoveredTile{};
        if (map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile)) {
            Tilemap::Coord coord{};
            bool validCoord = map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
            assert(validCoord);  // should always be true when hoveredTile != null

            if (editorPlaceTile) {
                map.Set(coord.x, coord.y, cursorTile, now);
            } else if (editorPickTile) {
                cursorTile = hoveredTile;
            } else if (editorFillTile) {
                map.Fill(coord.x, coord.y, cursorTile, now);
            }

            Vector2 drawPos{ (float)coord.x * TILE_W, (float)coord.y * TILE_W };
            map.DrawTile(cursorTile, drawPos);
            DrawRectangleLinesEx({ drawPos.x, drawPos.y, TILE_W, TILE_W }, 2, WHITE);
        }
    }
}
void Editor::DrawGroundOverlay_Wang(Tilemap &map, Camera2D &camera, double now)
{
}
void Editor::DrawGroundOverlay_Paths(Tilemap &map, Camera2D &camera, double now)
{
    auto &cursor = state.pathNodes.cursor;
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

    // Draw path edges
    for (uint32_t aiPathId = 0; aiPathId < map.paths.size(); aiPathId++) {
        AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
            uint32_t aiPathNodeNextIndex = map.GetNextPathNodeIndex(aiPathId, aiPathNodeIndex);
            AiPathNode *aiPathNode = map.GetPathNode(aiPathId, aiPathNodeIndex);
            AiPathNode *aiPathNodeNext = map.GetPathNode(aiPathId, aiPathNodeNextIndex);
            assert(aiPathNode);
            assert(aiPathNodeNext);
            DrawLine(
                aiPathNode->pos.x, aiPathNode->pos.y,
                aiPathNodeNext->pos.x, aiPathNodeNext->pos.y,
                LIGHTGRAY
            );
        }
    }

    // Draw path nodes
    const float pathRectRadius = 5;
    for (uint32_t aiPathId = 0; aiPathId < map.paths.size(); aiPathId++) {
        AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
            AiPathNode *aiPathNode = map.GetPathNode(aiPathId, aiPathNodeIndex);
            assert(aiPathNode);

            Rectangle nodeRect{
                aiPathNode->pos.x - pathRectRadius,
                aiPathNode->pos.y - pathRectRadius,
                pathRectRadius * 2,
                pathRectRadius * 2
            };

            Color color = aiPathNode->waitFor ? BLUE : RED;

            bool hover = !io.MouseCaptured() && dlb_CheckCollisionPointRec(cursorWorldPos, nodeRect);
            if (hover) {
                io.CaptureMouse();

                color = aiPathNode->waitFor ? SKYBLUE : PINK;

                if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    cursor.dragging = true;
                    cursor.dragPathId = aiPathId;
                    cursor.dragPathNodeIndex = aiPathNodeIndex;
                    cursor.dragStartPosition = aiPathNode->pos;
                }
            }

            if (cursor.dragging && cursor.dragPathNodeIndex == aiPathNodeIndex) {
                io.PushScope(IO::IO_EditorDrag);
                if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    io.CaptureMouse();
                    io.CaptureKeyboard();

                    color = aiPathNode->waitFor ? DARKBLUE : MAROON;

                    if (io.KeyPressed(KEY_ESCAPE)) {
                        aiPathNode->pos = cursor.dragStartPosition;
                        cursor = {};
                        io.CaptureKeyboard();
                    }

                } else {
                    cursor = {};
                }
                io.PopScope();
            }
            DrawRectangleRec(nodeRect, color);
        }
    }

    if (cursor.dragging) {
        io.PushScope(IO::IO_EditorDrag);

        Vector2 newNodePos = cursorWorldPos;
        if (io.KeyDown(KEY_LEFT_SHIFT)) {
            newNodePos.x = roundf(newNodePos.x / 8) * 8;
            newNodePos.y = roundf(newNodePos.y / 8) * 8;
        }
        Vector2SubtractValue(newNodePos, pathRectRadius);
        AiPath *aiPath = map.GetPath(cursor.dragPathId);
        if (aiPath) {
            AiPathNode *aiPathNode = map.GetPathNode(cursor.dragPathId, cursor.dragPathNodeIndex);
            assert(aiPathNode);
            aiPathNode->pos = newNodePos;
        }

        io.PopScope();
    }
}
void Editor::DrawGroundOverlay_Warps(Tilemap &map, Camera2D &camera, double now)
{
    for (const Warp &warp : map.warps) {
        DrawRectangleRec(warp.collider, Fade(SKYBLUE, 0.7f));
    }
}

void Editor::DrawEntityOverlays(Tilemap &map, Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorEntityOverlay);

    if (active) {
        switch (mode) {
            case EditMode_Tiles: {
                break;
            }
            case EditMode_Wang: {
                break;
            }
            case EditMode_Paths: {
                break;
            }
            case EditMode_Warps: {
                break;
            }
            case EditMode_Entities: {
                DrawEntityOverlay_Collision(map, camera, now);
                break;
            }
        }
    }

    io.PopScope();
}
void Editor::DrawEntityOverlay_Collision(Tilemap &map, Camera2D &camera, double now)
{
    for (int entityId = 0; entityId < SV_MAX_ENTITIES; entityId++) {
        Entity &entity = map.entities[entityId];
        if (entity.type) {
            // [Debug] Draw entity texture rect
            //DrawRectangleRec(map.EntityRect(entityId), Fade(SKYBLUE, 0.7f));

            // [Debug] Draw colliders
            if (state.showColliders) {
                AspectCollision &collider = map.collision[entityId];
                if (collider.radius) {
                    DrawCircle(
                        entity.position.x, entity.position.y,
                        collider.radius,
                        collider.colliding ? Fade(RED, 0.5) : Fade(LIME, 0.5)
                    );
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
}

UIState Editor::DrawUI(Vector2 position, Tilemap &map, double now)
{
    io.PushScope(IO::IO_EditorUI);

    UIState state{};
    if (active) {
        state = DrawUI_ActionBar(position, map, now);
        if (state.hover) {
            io.CaptureMouse();
        }
    }

    io.PopScope();
    return state;
}
UIState Editor::DrawUI_ActionBar(Vector2 position, Tilemap &map, double now)
{
    UIStyle uiActionBarStyle{};
    UI uiActionBar{ position, uiActionBarStyle };

    // TODO: UI::Panel
    UIState uiState{};
    const Rectangle actionBarRect{ position.x, position.y, 430.0f, GetRenderHeight() - 8.0f };
#if 0
    DrawRectangleRounded(actionBarRect, 0.15f, 8, ASESPRITE_BEIGE);
    DrawRectangleRoundedLines(actionBarRect, 0.15f, 8, 2.0f, BLACK);
#else
    DrawRectangleRec(actionBarRect, GREEN_DESAT); //ASESPRITE_BEIGE);
    DrawRectangleLinesEx(actionBarRect, 2.0f, BLACK);
#endif
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);

    const char *mapFileFilter[1] = { "*.dat" };
    static std::string openRequest;
    static std::string saveAsRequest;

    UIState openButton = uiActionBar.Button("Open");
    if (openButton.released) {
        std::string filename = map.filename;
        std::thread openFileThread([filename, mapFileFilter]{
            const char *openRequestBuf = tinyfd_openFileDialog(
                "Open File",
                filename.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.dat)",
                0
            );
            if (openRequestBuf) openRequest = openRequestBuf;
        });
        openFileThread.detach();
    }
    if (openRequest.size()) {
        Err err = map.Load(openRequest, now);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to load file %s. %s\n", openRequest, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        openRequest.clear();
    }

    UIState saveButton = uiActionBar.Button("Save");
    if (saveButton.released) {
        //map.SaveKV(map.filename + ".txt");
        Err err = map.Save(map.filename);
        if (err) {
            std::string filename = map.filename;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState saveAsButton = uiActionBar.Button("Save As");
    if (saveAsButton.released) {
        std::string filename = map.filename;
        std::thread saveAsThread([filename, mapFileFilter]{
            const char *saveAsRequestBuf = tinyfd_saveFileDialog(
                "Save File",
                filename.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.dat)");
            if (saveAsRequestBuf) saveAsRequest = saveAsRequestBuf;
        });
        saveAsThread.detach();
    }
    if (saveAsRequest.size()) {
        Err err = map.Save(saveAsRequest);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", saveAsRequest.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        saveAsRequest.clear();
    }

    UIState reloadButton = uiActionBar.Button("Reload");
    if (reloadButton.released) {
        Err err = map.Load(map.filename, now);
        if (err) {
            std::string filename = map.filename;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to reload file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }
    uiActionBar.Newline();

    UIState mapPath = uiActionBar.Text(GetFileName(map.filename.c_str()), WHITE);
    if (mapPath.released) {
        system("explorer maps");
    }
    uiActionBar.Newline();

    uiActionBar.Text("Show:");
    UIState showCollidersButton = uiActionBar.Button("Collision", state.showColliders, GRAY, MAROON);
    if (showCollidersButton.released) {
        state.showColliders = !state.showColliders;
    }
    UIState showTileIdsButton = uiActionBar.Button("Tile IDs", state.showTileIds, GRAY, LIGHTGRAY);
    if (showTileIdsButton.released) {
        state.showTileIds = !state.showTileIds;
    }

    uiActionBar.Newline();

    for (int i = 0; i < EditMode_Count; i++) {
        if (uiActionBar.Button(EditModeStr((EditMode)i), mode == i, BLUE, DARKBLUE).pressed) {
            mode = (EditMode)i;
        }
    }
    uiActionBar.Newline();

    uiActionBar.Text("--------------------------------------", LIGHTGRAY);
    uiActionBar.Newline();

    switch (mode) {
        case EditMode_Tiles: {
            DrawUI_TileActions(uiActionBar, map, now);
            break;
        }
        case EditMode_Wang: {
            DrawUI_Wang(uiActionBar, map, now);
            break;
        }
        case EditMode_Entities: {
            DrawUI_EntityActions(uiActionBar, map, now);
            break;
        }
        case EditMode_Paths: {
            DrawUI_PathActions(uiActionBar, map, now);
            break;
        }
        case EditMode_Warps: {
            DrawUI_WarpActions(uiActionBar, map, now);
            break;
        }
    }

    return uiState;
}
void Editor::DrawUI_TileActions(UI &uiActionBar, Tilemap &map, double now)
{
    const char *mapFileFilter[1] = { "*.png" };
    static const char *openRequest = 0;

    if (uiActionBar.Button("Change tileset", ColorBrightness(ORANGE, -0.2f)).released) {
        std::string filename = rnStringCatalog.GetString(map.textureId);
        std::thread openFileThread([filename, mapFileFilter]{
            openRequest = tinyfd_openFileDialog(
                "Open File",
                filename.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tileset (*.png)",
                0
            );
            });
        openFileThread.detach();
    }
    if (openRequest) {
        StringId newTextureId = rnStringCatalog.AddString(openRequest);
        Err err = Tilemap::ChangeTileset(map, newTextureId, now);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to load file %s. %s\n", openRequest, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        openRequest = 0;
    }
    uiActionBar.Newline();

    uiActionBar.Text("Edit:");
    UIState editCollisionButton = uiActionBar.Button("Collision", state.tiles.editCollision, GRAY, MAROON);
    if (editCollisionButton.released) {
        state.tiles.editCollision = !state.tiles.editCollision;
    }
    uiActionBar.Newline();

    DrawUI_Tilesheet(uiActionBar, map, now);
}
void Editor::DrawUI_Tilesheet(UI &uiActionBar, Tilemap &map, double now)
{
    // TODO: Support multi-select (big rectangle?), and figure out where this lives

    Texture mapTex = rnTextureCatalog.GetTexture(map.textureId);

    static Texture checkerboard{};
    if (checkerboard.width != mapTex.width || checkerboard.height != mapTex.height) {
        checkerboard = LoadTextureFromImage(
            GenImageChecked(mapTex.width, mapTex.height, 8, 8, GRAY, LIGHTGRAY)
        );
    }

    UIStyle blackBorderStyle{};
    blackBorderStyle.borderColor = BLACK;
    uiActionBar.PushStyle(blackBorderStyle);
    UIState sheet = uiActionBar.Image(checkerboard);
    DrawTextureEx(mapTex, sheet.contentTopLeft, 0, 1, WHITE);
    uiActionBar.PopStyle();

    Vector2 imgTL = sheet.contentTopLeft;

    // Draw collision overlay on tilesheet if we're in collision editing mode
    if (state.tiles.editCollision) {
        for (int i = 0; i < map.tileDefs.size(); i++) {
            if (map.tileDefs[i].collide) {
                Rectangle tileDefRectScreen = map.TileDefRect(i);
                tileDefRectScreen.x += imgTL.x;
                tileDefRectScreen.y += imgTL.y;
                tileDefRectScreen = RectShrink(tileDefRectScreen, 2);
                DrawRectangleLinesEx(tileDefRectScreen, 2.0f, MAROON);
            }
        }
    }

    if (sheet.hover) {
        Vector2 mousePos = GetMousePosition();
        Vector2 mouseRel = Vector2Subtract(mousePos, imgTL);

        //DrawTextEx(fntHackBold20, TextFormat("%f, %f\n", mouseRel.x, mouseRel.y),
        //    Vector2Add(GetMousePosition(), { 10, 10 }), fntHackBold20.baseSize, 1, YELLOW);

        // Draw hover highlight on tilesheet if mouse hovering a tile
        int tileX = (int)mouseRel.x / 32;
        int tileY = (int)mouseRel.y / 32;
        DrawRectangleLinesEx(
            { imgTL.x + tileX * TILE_W, imgTL.y + tileY * TILE_W, TILE_W, TILE_W },
            2,
            Fade(WHITE, 0.7f)
        );

        //DrawTextEx(fntHackBold20, TextFormat("%d, %d\n", tileX, tileY),
        //    Vector2Add(GetMousePosition(), { 10, 10 }), fntHackBold20.baseSize, 1, YELLOW);

        // If mouse pressed, select tile, or change collision data, depending on mode
        if (sheet.pressed) {
            int tileIdx = tileY * (mapTex.width / TILE_W) + tileX;
            if (tileIdx >= 0 && tileIdx < map.tileDefs.size()) {
                switch (state.tiles.editCollision) {
                    case false: {
                        state.tiles.cursor.tileDefId = tileIdx;
                        break;
                    }
                    case true: {
                        TileDef &tileDef = map.tileDefs[tileIdx];
                        tileDef.collide = !tileDef.collide;
                        break;
                    }
                }
            } else {
                assert("!wut da heck, how you select a tile that's not in the map at all?");
            }
        }
    }

    // Draw highlight around currently styleSelected tiledef in draw mode
    if (state.tiles.cursor.tileDefId >= 0) {
        Rectangle tileDefRectScreen = map.TileDefRect(state.tiles.cursor.tileDefId);
        tileDefRectScreen.x += imgTL.x;
        tileDefRectScreen.y += imgTL.y;
        DrawRectangleLinesEx(tileDefRectScreen, 2, WHITE);
    }
}
void Editor::DrawUI_Wang(UI &uiActionBar, Tilemap &map, double now)
{
    if (uiActionBar.Button("Generate template").pressed) {
        Err err = WangTileset::GenerateTemplate("resources/wang/template.png");
        if (err) {
            // TODO: Show the user
            printf("[editor] Failed to generate wang tileset template\n");
        }
    }
    uiActionBar.Newline();

    WangTileset &wangTileset = state.wang.wangTileset;
    WangMap &wangMap = state.wang.wangMap;

    UIStyle uiWangStyle4x{};
    uiWangStyle4x.scale = 4;
    uiActionBar.PushStyle(uiWangStyle4x);

    UIStyle styleSelected{ uiWangStyle4x };
    styleSelected.borderColor = WHITE;

    static int hTex = -1;
    static int vTex = -1;
    for (int i = 0; i < wangTileset.ts.num_h_tiles; i++) {
        stbhw_tile *wangTile = wangTileset.ts.h_tiles[i];
        Rectangle srcRect{
            (float)wangTile->x,
            (float)wangTile->y,
            (float)wangTileset.ts.short_side_len * 2,
            (float)wangTileset.ts.short_side_len
        };
        bool stylePushed = false;
        if (i == hTex) {
            uiActionBar.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (uiActionBar.Image(wangTileset.thumbnail, srcRect).pressed) {
            hTex = hTex == i ? -1 : i;
            vTex = -1;
        }
        if (stylePushed) {
            uiActionBar.PopStyle();
        }
    }
    Vector2 cursorEndOfFirstLine = uiActionBar.CursorScreen();
    uiActionBar.Newline();

    for (int i = 0; i < wangTileset.ts.num_v_tiles; i++) {
        stbhw_tile *wangTile = wangTileset.ts.v_tiles[i];
        Rectangle srcRect{
            (float)wangTile->x,
            (float)wangTile->y,
            (float)wangTileset.ts.short_side_len,
            (float)wangTileset.ts.short_side_len * 2
        };
        bool stylePushed = false;
        if (i == vTex) {
            uiActionBar.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (uiActionBar.Image(wangTileset.thumbnail, srcRect).pressed) {
            hTex = -1;
            vTex = vTex == i ? -1 : i;
        }
        if (stylePushed) {
            uiActionBar.PopStyle();
        }
    }
    uiActionBar.Newline();

    if (uiActionBar.Button("Export tileset").pressed) {
        ExportImage(wangTileset.ts.img, wangTileset.filename.c_str());
    }
    uiActionBar.Newline();
    uiActionBar.Image(wangTileset.thumbnail);
    uiActionBar.Newline();

    uiActionBar.PopStyle();
    UIStyle uiWangStyle2x{};
    uiWangStyle2x.scale = 2;
    uiActionBar.PushStyle(uiWangStyle2x);

    if (uiActionBar.Button("Re-generate Map").pressed) {
        wangTileset.GenerateMap(map.width, map.height, map, wangMap);
    }
    uiActionBar.Newline();

    static Tilemap wangTilemap{};
    if (uiActionBar.Image(wangMap.colorized).pressed) {
        map.SetFromWangMap(wangMap, now);
    }

    if (hTex >= 0 || vTex >= 0) {
        Tile selectedTile = state.tiles.cursor.tileDefId;
        static double lastUpdatedAt = 0;
        static double lastChangedAt = 0;
        const double updateDelay = 0.02;

        Rectangle wangBg{ cursorEndOfFirstLine.x + 8, cursorEndOfFirstLine.y + 8, 560, 560 };
        DrawRectangleRec(wangBg, Fade(BLACK, 0.5f));

        UIStyle uiWangTileStyle{};
        uiWangTileStyle.margin = 0;
        uiWangTileStyle.imageBorderThickness = 1;
        UI uiWangTile{ { wangBg.x + 8, wangBg.y + 8 }, uiWangTileStyle };
        if (hTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.h_tiles[hTex];
            for (int y = 0; y < wangTileset.ts.short_side_len; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len*2; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    uint8_t *pixel = &((uint8_t *)wangTileset.ts.img.data)[(templateY * wangTileset.ts.img.width + templateX) * 3];
                    uint8_t tile = pixel[0] < map.tileDefs.size() ? pixel[0] : 0;

                    Texture mapTex = rnTextureCatalog.GetTexture(map.textureId);
                    const Rectangle tileRect = map.TileDefRect(tile);
                    if (uiWangTile.Image(mapTex, tileRect).down) {
                        pixel[0] = selectedTile; //^ (selectedTile*55);
                        pixel[1] = selectedTile; //^ (selectedTile*55);
                        pixel[2] = selectedTile; //^ (selectedTile*55);
                        lastChangedAt = now;
                    }
                }
                uiWangTile.Newline();
            }
        } else if (vTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.v_tiles[vTex];
            for (int y = 0; y < wangTileset.ts.short_side_len*2; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    uint8_t *pixel = &((uint8_t *)wangTileset.ts.img.data)[(templateY * wangTileset.ts.img.width + templateX) * 3];
                    uint8_t tile = pixel[0] < map.tileDefs.size() ? pixel[0] : 0;

                    Texture mapTex = rnTextureCatalog.GetTexture(map.textureId);
                    const Rectangle tileRect = map.TileDefRect(tile);
                    if (uiWangTile.Image(mapTex, tileRect).down) {
                        pixel[0] = selectedTile; //^ (selectedTile*55);
                        pixel[1] = selectedTile; //^ (selectedTile*55);
                        pixel[2] = selectedTile; //^ (selectedTile*55);
                        lastChangedAt = now;
                    }
                }
                uiWangTile.Newline();
            }
        }

        // Update if dirty on a slight delay (to make dragging more efficient)
        if (lastChangedAt > lastUpdatedAt && (now - lastChangedAt) > updateDelay) {
            UnloadTexture(wangTileset.thumbnail);
            wangTileset.thumbnail = wangTileset.GenerateColorizedTexture(wangTileset.ts.img, map);
            lastUpdatedAt = now;
        }
    }
    uiActionBar.PopStyle();
}
void Editor::DrawUI_EntityActions(UI &uiActionBar, Tilemap &map, double now)
{
    if (uiActionBar.Button("Despawn all", MAROON).pressed) {
        for (uint32_t entityId = 0; entityId < ARRAY_SIZE(map.entities); entityId++) {
            const Entity &entity = map.entities[entityId];
            if (entity.type) {
                map.DespawnEntity(entityId, now);
            }
        }
    }
}
void Editor::DrawUI_PathActions(UI &uiActionBar, Tilemap &map, double now)
{
    if (state.pathNodes.cursor.dragging) {
        printf("dragging\n");
    }
    uiActionBar.Text(TextFormat(
        "drag: %s, path: %d, node: %d",
        state.pathNodes.cursor.dragging ? "true" : "false",
        state.pathNodes.cursor.dragPathId,
        state.pathNodes.cursor.dragPathNodeIndex
    ));
}
void Editor::DrawUI_WarpActions(UI &uiActionBar, Tilemap &map, double now)
{
    if (uiActionBar.Button("Delete all warps", MAROON).pressed) {
        map.warps.clear();
    }
}