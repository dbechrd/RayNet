#include "../common/collision.h"
#include "../common/common.h"
#include "../common/entity_db.h"
#include "../common/io.h"
#include "../common/texture_catalog.h"
#include "../common/tilemap.h"
#include "../common/ui/ui.h"
#include "editor.h"
#include "game_server.h"
#include "stb_herringbone_wang_tile.h"
#include "tinyfiledialogs.h"

const char *EditModeStr(EditMode mode)
{
    switch (mode) {
        case EditMode_Maps:      return "Maps";
        case EditMode_Tiles:     return "Tiles";
        case EditMode_Wang:      return "Wang";
        case EditMode_Paths:     return "Paths";
        case EditMode_Warps:     return "Warps";
        case EditMode_Entities:  return "Entities";
        case EditMode_SfxFiles:  return "Sfx";
        case EditMode_PackFiles: return "Pack";
        default: return "<null>";
    }
}

Err Editor::Init(void)
{
    Err err = RN_SUCCESS;
    err = state.wang.wangTileset.Load("resources/wang/tileset2x2.png");
    return err;
}

void Editor::HandleInput(Camera2D &camera)
{
    io.PushScope(IO::IO_Editor);

    if (io.KeyPressed(KEY_GRAVE)) {
        active = !active;
    }

    if (active) {
        if (io.KeyPressed(KEY_ESCAPE)) {
            if (UI::UnfocusActiveEditor()) {
                // that was all escape should do this frame
            } else {
                active = false;
            }
        }
        if (io.KeyDown(KEY_LEFT_CONTROL) && io.KeyPressed(KEY_ZERO)) {
            camera.zoom = 1.0f;
        }

        io.CaptureKeyboard();
    } else {
        UI::UnfocusActiveEditor();
    }

    io.PopScope();
}

void Editor::DrawGroundOverlays(Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorGroundOverlay);

    // Draw tile collision layer
    if (state.showColliders) {
        map->DrawColliders(camera);
    }
    if (state.showTileIds) {
        map->DrawTileIds(camera);
    }

    if (active) {
        switch (mode) {
            case EditMode_Tiles: {
                DrawGroundOverlay_Tiles(camera, now);
                break;
            }
            case EditMode_Wang: {
                DrawGroundOverlay_Wang(camera, now);
                break;
            }
            case EditMode_Paths: {
                DrawGroundOverlay_Paths(camera, now);
                break;
            }
            case EditMode_Warps: {
                DrawGroundOverlay_Warps(camera, now);
                break;
            }
            case EditMode_Entities: {
                break;
            }
        }
    }

    io.PopScope();
}
void Editor::DrawGroundOverlay_Tiles(Camera2D &camera, double now)
{
    if (!io.MouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPlaceTile = io.MouseButtonDown(MOUSE_BUTTON_LEFT);
        const bool editorPickTile = io.MouseButtonDown(MOUSE_BUTTON_MIDDLE);
        const bool editorFillTile = io.KeyPressed(KEY_F);

        Tile &cursorTile = state.tiles.cursor.tileDefId;
        Tile hoveredTile{};
        if (map->AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile)) {
            Tilemap::Coord coord{};
            bool validCoord = map->WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
            assert(validCoord);  // should always be true when hoveredTile != null

            if (editorPlaceTile) {
                map->Set(coord.x, coord.y, cursorTile, now);
            } else if (editorPickTile) {
                cursorTile = hoveredTile;
            } else if (editorFillTile) {
                map->Fill(coord.x, coord.y, cursorTile, now);
            }

            Vector2 drawPos{ (float)coord.x * TILE_W, (float)coord.y * TILE_W };
            const Texture tex = rnTextureCatalog.GetTexture(map->textureId);
            map->DrawTile(tex, cursorTile, drawPos);
            DrawRectangleLinesEx({ drawPos.x, drawPos.y, TILE_W, TILE_W }, 2, WHITE);
        }
    }
}
void Editor::DrawGroundOverlay_Wang(Camera2D &camera, double now)
{
}
void Editor::DrawGroundOverlay_Paths(Camera2D &camera, double now)
{
    auto &cursor = state.pathNodes.cursor;
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

    // Draw path edges
    for (uint32_t aiPathId = 0; aiPathId < map->paths.size(); aiPathId++) {
        AiPath *aiPath = map->GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
            uint32_t aiPathNodeNextIndex = map->GetNextPathNodeIndex(aiPathId, aiPathNodeIndex);
            AiPathNode *aiPathNode = map->GetPathNode(aiPathId, aiPathNodeIndex);
            AiPathNode *aiPathNodeNext = map->GetPathNode(aiPathId, aiPathNodeNextIndex);
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
    for (uint32_t aiPathId = 0; aiPathId < map->paths.size(); aiPathId++) {
        AiPath *aiPath = map->GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = 0; aiPathNodeIndex < aiPath->pathNodeIndexCount; aiPathNodeIndex++) {
            AiPathNode *aiPathNode = map->GetPathNode(aiPathId, aiPathNodeIndex);
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
        AiPath *aiPath = map->GetPath(cursor.dragPathId);
        if (aiPath) {
            AiPathNode *aiPathNode = map->GetPathNode(cursor.dragPathId, cursor.dragPathNodeIndex);
            assert(aiPathNode);
            aiPathNode->pos = newNodePos;
        }

        io.PopScope();
    }
}
void Editor::DrawGroundOverlay_Warps(Camera2D &camera, double now)
{
    for (const data::AspectWarp &warp : data::pack1.warp) {
        DrawRectangleRec(warp.collider, Fade(SKYBLUE, 0.7f));
    }
}

void Editor::DrawEntityOverlays(Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorEntityOverlay);

    data::Entity *selectedEntity = entityDb->FindEntity(state.entities.selectedId);
    if (selectedEntity && selectedEntity->mapId == map->id) {
        DrawTextEx(fntSmall, TextFormat("[selected in editor]\n%u", selectedEntity->id),
            selectedEntity->position, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
    }

    if (state.showEntityIds) {
        for (data::Entity &entity : entityDb->entities) {
            if (entity.mapId == map->id && entity.id != state.entities.selectedId) {
                entityDb->DrawEntityIds(entity.id, camera);
            }
        }
    }

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
                DrawEntityOverlay_Collision(camera, now);
                break;
            }
        }
    }

    io.PopScope();
}
void Editor::DrawEntityOverlay_Collision(Camera2D &camera, double now)
{
    for (data::Entity &entity : entityDb->entities) {
        if (entity.mapId != map->id) {
            continue;
        }
        assert(entity.id && entity.type);

        // [Debug] Draw entity texture rect
        //DrawRectangleRec(map->EntityRect(i), Fade(SKYBLUE, 0.7f));

        // [Debug] Draw colliders
        if (state.showColliders) {
            size_t entityIndex = entityDb->FindEntityIndex(entity.id);
            data::AspectCollision &collider = entityDb->collision[entityIndex];
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

UIState Editor::DrawUI(Vector2 position, GameServer &server, double now)
{
    io.PushScope(IO::IO_EditorUI);

    UIState state{};
    if (active) {
        state = DrawUI_ActionBar(position, server, now);
        if (state.hover) {
            io.CaptureMouse();
        }
    }

    io.PopScope();
    return state;
}
UIState Editor::DrawUI_ActionBar(Vector2 position, GameServer &server, double now)
{
    UIStyle uiActionBarStyle{};
    UI uiActionBar{ position, uiActionBarStyle };

    // TODO: UI::Panel
    UIState uiState{};
    const Rectangle actionBarRect{ position.x, position.y, 430.0f, (float)GetRenderHeight() };
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
        std::string filename = map->name;
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
        Tilemap *openedMap = server.FindOrLoadMap(openRequest);
        if (!openedMap) {
            std::thread errorThread([]{
                const char *msg = TextFormat("Failed to load file %s.\n", openRequest);
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        openRequest.clear();
    }

    UIState saveButton = uiActionBar.Button("Save");
    if (saveButton.released) {
        //map->SaveKV(map->filename + ".txt");
        Err err = map->Save(map->name);
        if (err) {
            std::string filename = map->name;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState saveAsButton = uiActionBar.Button("Save As");
    if (saveAsButton.released) {
        std::string filename = map->name;
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
        Err err = map->Save(saveAsRequest);
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
        Err err = map->Load(map->name);
        if (err) {
            std::string filename = map->name;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to reload file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }
    uiActionBar.Newline();

    UIState mapPath = uiActionBar.Text(GetFileName(map->name.c_str()), WHITE);
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
    UIState showEntityIdsButton = uiActionBar.Button("Entity IDs", state.showEntityIds, GRAY, LIGHTGRAY);
    if (showEntityIdsButton.released) {
        state.showEntityIds = !state.showEntityIds;
    }

    uiActionBar.Newline();

    for (int i = 0; i < EditMode_Count; i++) {
        if (uiActionBar.Button(EditModeStr((EditMode)i), mode == i, BLUE, DARKBLUE).pressed) {
            mode = (EditMode)i;
        }
        if (i % 6 == 5) {
            uiActionBar.Newline();
        }
    }
    uiActionBar.Newline();

    uiActionBar.Text("--------------------------------------", LIGHTGRAY);
    uiActionBar.Newline();

    switch (mode) {
        case EditMode_Maps: {
            DrawUI_MapActions(uiActionBar, server, now);
            break;
        }
        case EditMode_Tiles: {
            DrawUI_TileActions(uiActionBar, now);
            break;
        }
        case EditMode_Wang: {
            DrawUI_Wang(uiActionBar, now);
            break;
        }
        case EditMode_Paths: {
            DrawUI_PathActions(uiActionBar, now);
            break;
        }
        case EditMode_Warps: {
            DrawUI_WarpActions(uiActionBar, now);
            break;
        }
        case EditMode_Entities: {
            DrawUI_EntityActions(uiActionBar, now);
            break;
        }
        case EditMode_SfxFiles: {
            DrawUI_SfxFiles(uiActionBar, now);
            break;
        }
        case EditMode_PackFiles: {
            DrawUI_PackFiles(uiActionBar, now);
            break;
        }
    }

    return uiState;
}
void Editor::DrawUI_MapActions(UI &uiActionBar, GameServer &server, double now)
{
    for (Tilemap *map : server.maps) {
        if (uiActionBar.Button(TextFormat("[%d] %s", map->id, map->name.c_str())).pressed) {
            this->map = map;
        }
        uiActionBar.Newline();
    }
}
void Editor::DrawUI_TileActions(UI &uiActionBar, double now)
{
    const char *mapFileFilter[1] = { "*.png" };
    static const char *openRequest = 0;

    std::string tilesetPath = rnStringCatalog.GetString(map->textureId);
    uiActionBar.Text(tilesetPath.c_str());
    if (uiActionBar.Button("Change tileset", ColorBrightness(ORANGE, -0.2f)).released) {
        std::thread openFileThread([tilesetPath, mapFileFilter]{
            openRequest = tinyfd_openFileDialog(
                "Open File",
                tilesetPath.c_str(),
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
        Err err = Tilemap::ChangeTileset(*map, newTextureId, now);
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

    uiActionBar.Text("Flag");

    auto &tileEditMode = state.tiles.tileEditMode;
    if (uiActionBar.Button("Select",
        tileEditMode == TileEditMode_Select, GRAY, SKYBLUE).pressed)
    {
        tileEditMode = TileEditMode_Select;
    }
    if (uiActionBar.Button("Collision",
        tileEditMode == TileEditMode_Collision, GRAY, SKYBLUE).pressed)
    {
        tileEditMode = TileEditMode_Collision;
    }
    if (uiActionBar.Button("Auto-tile Mask",
        tileEditMode == TileEditMode_AutoTileMask, GRAY, SKYBLUE).pressed)
    {
        tileEditMode = TileEditMode_AutoTileMask;
    }
    uiActionBar.Newline();

    DrawUI_Tilesheet(uiActionBar, now);
}
void DrawRectangleRectOffset(const Rectangle &rect, Vector2 &offset, Color color)
{
    Rectangle offsetRect = rect;
    offsetRect.x += offset.x;
    offsetRect.y += offset.y;
    DrawRectangleRec(offsetRect, color);
}
void Editor::DrawUI_Tilesheet(UI &uiActionBar, double now)
{
    // TODO: Support multi-select (big rectangle?), and figure out where this lives

    Texture mapTex = rnTextureCatalog.GetTexture(map->textureId);

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
    Vector2 imgTL{ sheet.contentRect.x, sheet.contentRect.y };

    DrawTextureEx(mapTex, imgTL, 0, 1, WHITE);
    uiActionBar.PopStyle();

    // Draw collision overlay on tilesheet if we're in collision editing mode
    switch (state.tiles.tileEditMode) {
        case TileEditMode_Collision: {
            for (int i = 0; i < map->tileDefs.size(); i++) {
                if (map->tileDefs[i].collide) {
                    Rectangle tileDefRectScreen = map->TileDefRect(i);
                    tileDefRectScreen.x += imgTL.x;
                    tileDefRectScreen.y += imgTL.y;
                    tileDefRectScreen = RectShrink(tileDefRectScreen, 2);
                    DrawRectangleLinesEx(tileDefRectScreen, 2.0f, MAROON);
                }
            }
            break;
        }
        case TileEditMode_AutoTileMask: {
            for (int i = 0; i < map->tileDefs.size(); i++) {
                const int tileThird = TILE_W / 3;
                Rectangle tileDefRectScreen = map->TileDefRect(i);
                tileDefRectScreen.x += imgTL.x + 1;
                tileDefRectScreen.y += imgTL.y + 1;
                tileDefRectScreen.width = tileThird;
                tileDefRectScreen.height = tileThird;

                const Color color = Fade(MAROON, 0.7f);

                Vector2 cursor{};
                uint8_t mask = map->tileDefs[i].autoTileMask;
                if (mask & 0b10000000) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                if (mask & 0b01000000) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                if (mask & 0b00100000) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x = 0;
                cursor.y += tileThird;

                if (mask & 0b00010000) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                if (mask & 0b00001000) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x = 0;
                cursor.y += tileThird;

                if (mask & 0b00000100) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                if (mask & 0b00000010) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
                cursor.x += tileThird;

                if (mask & 0b00000001) DrawRectangleRectOffset(tileDefRectScreen, cursor, color);
            }
            break;
        };
    }

    const Vector2 mousePos = GetMousePosition();
    if (sheet.hover && dlb_CheckCollisionPointRec(mousePos, sheet.contentRect)) {
        const Vector2 mouseRel = Vector2Subtract(mousePos, imgTL);

        //DrawTextEx(fntHackBold20, TextFormat("%f, %f\n", mouseRel.x, mouseRel.y),
        //    Vector2Add(GetMousePosition(), { 10, 10 }), fntHackBold20.baseSize, 1, YELLOW);

        // Draw hover highlight on tilesheet if mouse hovering a tile
        const int tileX = (int)mouseRel.x / TILE_W;
        const int tileY = (int)mouseRel.y / TILE_W;
        if (state.tiles.tileEditMode != TileEditMode_AutoTileMask) {
            DrawRectangleLinesEx(
                { imgTL.x + tileX * TILE_W, imgTL.y + tileY * TILE_W, TILE_W, TILE_W },
                2,
                Fade(WHITE, 0.7f)
            );
        }

        //DrawTextEx(fntHackBold20, TextFormat("%d, %d\n", tileX, tileY),
        //    Vector2Add(GetMousePosition(), { 10, 10 }), fntHackBold20.baseSize, 1, YELLOW);

        // If mouse pressed, select tile, or change collision data, depending on mode
        if (sheet.pressed) {
            const int tileIdx = tileY * (mapTex.width / TILE_W) + tileX;
            if (tileIdx >= 0 && tileIdx < map->tileDefs.size()) {
                switch (state.tiles.tileEditMode) {
                    case TileEditMode_Select: {
                        state.tiles.cursor.tileDefId = tileIdx;
                        break;
                    }
                    case TileEditMode_Collision: {
                        TileDef &tileDef = map->tileDefs[tileIdx];
                        tileDef.collide = !tileDef.collide;
                        break;
                    }
                    case TileEditMode_AutoTileMask: {
                        const int tileXRemainder = (int)mouseRel.x % TILE_W - 1;
                        const int tileYRemainder = (int)mouseRel.y % TILE_W - 1;
                        if (tileXRemainder < 0 || tileXRemainder > 29 ||
                            tileYRemainder < 0 || tileYRemainder > 29)
                        {
                            break;
                        }
                        const int tileXSegment = tileXRemainder / (TILE_W / 3);
                        const int tileYSegment = tileYRemainder / (TILE_W / 3);

                        const int tileSegment = tileYSegment * 3 + tileXSegment;
                        //printf("x: %d, y: %d, s: %d\n", tileXSegment, tileYSegment, tileSegment);
                        TileDef &tileDef = map->tileDefs[tileIdx];
                        switch (tileSegment) {
                            case 0: tileDef.autoTileMask ^= 0b10000000; break;
                            case 1: tileDef.autoTileMask ^= 0b01000000; break;
                            case 2: tileDef.autoTileMask ^= 0b00100000; break;
                            case 3: tileDef.autoTileMask ^= 0b00010000; break;
                            case 4: tileDef.autoTileMask ^= 0b00000000; break;
                            case 5: tileDef.autoTileMask ^= 0b00001000; break;
                            case 6: tileDef.autoTileMask ^= 0b00000100; break;
                            case 7: tileDef.autoTileMask ^= 0b00000010; break;
                            case 8: tileDef.autoTileMask ^= 0b00000001; break;
                        }
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
        Rectangle tileDefRectScreen = map->TileDefRect(state.tiles.cursor.tileDefId);
        tileDefRectScreen.x += imgTL.x;
        tileDefRectScreen.y += imgTL.y;
        DrawRectangleLinesEx(tileDefRectScreen, 2, WHITE);
    }
}
void Editor::DrawUI_Wang(UI &uiActionBar, double now)
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
        wangTileset.GenerateMap(map->width, map->height, wangMap);
    }
    uiActionBar.Newline();

    static Tilemap wangTilemap{};
    if (uiActionBar.Image(wangMap.colorized).pressed) {
        map->SetFromWangMap(wangMap, now);
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

        uint8_t *imgData = (uint8_t *)wangTileset.ts.img.data;

        if (hTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.h_tiles[hTex];
            for (int y = 0; y < wangTileset.ts.short_side_len; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len*2; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < map->tileDefs.size() ? pixel[0] : 0;

                    Texture mapTex = rnTextureCatalog.GetTexture(map->textureId);
                    const Rectangle tileRect = map->TileDefRect(tile);
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
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < map->tileDefs.size() ? pixel[0] : 0;

                    Texture mapTex = rnTextureCatalog.GetTexture(map->textureId);
                    const Rectangle tileRect = map->TileDefRect(tile);
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
            wangTileset.thumbnail = wangTileset.GenerateColorizedTexture(wangTileset.ts.img);
            lastUpdatedAt = now;
        }
    }
    uiActionBar.PopStyle();
}

void Editor::DrawUI_PathActions(UI &uiActionBar, double now)
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
void Editor::DrawUI_WarpActions(UI &uiActionBar, double now)
{
    //if (uiActionBar.Button("Delete all warps", MAROON).pressed) {
    //    map->warps.clear();
    //}
    //uiActionBar.Newline();

    if (uiActionBar.Button("Add", DARKGREEN).pressed) {
        //map->warps.push_back({});
    }
    uiActionBar.Newline();

    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{};
    uiActionBar.Textbox(txtSearch, filter);
    uiActionBar.Newline();

    //for (data::AspectWarp &warp : data::pack1.warp) {
    //    //if (!warp.id) {
    //    //    continue;
    //    //}
    //
    //    Color bgColor = warp.id == state.sfxFiles.selectedSfx ? SKYBLUE : BLUE;
    //    const char *idStr = sfxFile.path.c_str();
    //    if (!StrFilter(idStr, filter.c_str())) {
    //        continue;
    //    }
    //
    //    if (uiActionBar.Text(idStr, WHITE, bgColor).down) {
    //        state.sfxFiles.selectedSfx = sfxFile.id;
    //    }
    //    uiActionBar.Newline();
    //}

    for (data::AspectWarp &warp : data::pack1.warp) {
        uiActionBar.Text("collider");
        uiActionBar.Newline();

        uiActionBar.PushWidth(80);

        uiActionBar.Text("x");
        uiActionBar.Text("y");
        uiActionBar.Text("width");
        uiActionBar.Text("height");
        uiActionBar.Newline();

        static STB_TexteditState txtColliderX{};
        uiActionBar.TextboxFloat(txtColliderX, warp.collider.x);
        static STB_TexteditState txtColliderY{};
        uiActionBar.TextboxFloat(txtColliderY, warp.collider.y);
        static STB_TexteditState txtColliderW{};
        uiActionBar.TextboxFloat(txtColliderW, warp.collider.width);
        static STB_TexteditState txtColliderH{};
        uiActionBar.TextboxFloat(txtColliderH, warp.collider.height);
        uiActionBar.Newline();

        uiActionBar.Text("destX");
        uiActionBar.Text("destY");
        uiActionBar.Newline();

        static STB_TexteditState txtDestX{};
        uiActionBar.TextboxFloat(txtDestX, warp.destPos.x);
        static STB_TexteditState txtDestY{};
        uiActionBar.TextboxFloat(txtDestY, warp.destPos.y);
        uiActionBar.Newline();

        uiActionBar.PopStyle();

        uiActionBar.Text("destMap");
        uiActionBar.Newline();
        static STB_TexteditState txtDestMap{};
        uiActionBar.Textbox(txtDestMap, warp.destMap);
        uiActionBar.Newline();

        uiActionBar.Text("templateMap");
        uiActionBar.Newline();
        static STB_TexteditState txtTemplateMap{};
        uiActionBar.Textbox(txtTemplateMap, warp.templateMap);
        uiActionBar.Newline();

        uiActionBar.Text("templateTileset");
        uiActionBar.Newline();
        static STB_TexteditState txtTemplateTileset{};
        uiActionBar.Textbox(txtTemplateTileset, warp.templateTileset);
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();
}
void Editor::DrawUI_EntityActions(UI &uiActionBar, double now)
{
    if (uiActionBar.Button("Despawn all", MAROON).pressed) {
        for (const data::Entity &entity : entityDb->entities) {
            if (entity.type == data::ENTITY_PLAYER || entity.mapId != map->id) {
                continue;
            }
            assert(entity.id && entity.type);
            entityDb->DespawnEntity(entity.id, now);
        }
    }

    if (uiActionBar.Button(TextFormat("Despawn Test %d", state.entities.testId)).pressed) {
        state.entities.testId++;
    };
    uiActionBar.Newline();
    uiActionBar.Space({ 0, 4 });

    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{ "*" };
    uiActionBar.Textbox(txtSearch, filter);
    uiActionBar.Newline();

    for (uint32_t i = 0; i < SV_MAX_ENTITIES; i++) {
        data::Entity &entity = entityDb->entities[i];
        if (!entity.id || entity.mapId != map->id) {
            continue;
        }

        const char *idStr = TextFormat("[%d] %s", entity.id, EntityTypeStr(entity.type));
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        Color bgColor = entity.id == state.entities.selectedId ? ORANGE : BLUE;
        if (uiActionBar.Text(idStr, WHITE, bgColor).down) {
            state.entities.selectedId = entity.id;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.entities.selectedId) {
        size_t entityIndex = entityDb->FindEntityIndex(state.entities.selectedId);
        data::Entity &entity = entityDb->entities[entityIndex];
        data::AspectCombat    &eCombat    = entityDb->combat    [entityIndex];
        data::AspectCollision &eCollision = entityDb->collision [entityIndex];
        data::AspectDialog    &eDialog    = entityDb->dialog    [entityIndex];
        data::AspectLife      &eLife      = entityDb->life      [entityIndex];
        data::AspectPathfind  &ePathfind  = entityDb->pathfind  [entityIndex];
        data::AspectPhysics   &ePhysics   = entityDb->physics   [entityIndex];
        data::AspectSprite    &eSprite    = entityDb->sprite    [entityIndex];
        data::AspectWarp      &eWarp      = entityDb->warp      [entityIndex];

        const int labelWidth = 100;

        ////////////////////////////////////////////////////////////////////////
        // Entity
        uiActionBar.Label("id", labelWidth);
        uiActionBar.Text(TextFormat("%d", entity.id));
        uiActionBar.Newline();

        uiActionBar.Label("type", labelWidth);
        uiActionBar.Text(EntityTypeStr(entity.type));
        uiActionBar.Newline();

        uiActionBar.Label("position", labelWidth);
        uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeDefault);
        static STB_TexteditState txtPosX{};
        uiActionBar.TextboxFloat(txtPosX, entity.position.x, 80);
        uiActionBar.PopStyle();
        uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeDefault);
        static STB_TexteditState txtPosY{};
        uiActionBar.TextboxFloat(txtPosY, entity.position.y, 80);
        uiActionBar.PopStyle();
        uiActionBar.Newline();

        ////////////////////////////////////////////////////////////////////////
        // Combat
        uiActionBar.Label("attk cooldown", labelWidth);
        const float attackCooldownLeft = MAX(0, eCombat.attackCooldown - (now - eCombat.lastAttackedAt));
        uiActionBar.Text(TextFormat("%.3f", attackCooldownLeft));
        uiActionBar.Newline();

        ////////////////////////////////////////////////////////////////////////
        // Collision
        uiActionBar.Label("radius", labelWidth);
        static STB_TexteditState txtRadius{};
        uiActionBar.TextboxFloat(txtRadius, eCollision.radius, 80);
        uiActionBar.Newline();

        uiActionBar.Label("colliding", labelWidth);
        if (eCollision.colliding) {
            uiActionBar.Text("True", RED);
        } else {
            uiActionBar.Text("False", WHITE);
        }
        uiActionBar.Newline();

        uiActionBar.Label("onWarp", labelWidth);
        if (eCollision.onWarp) {
            uiActionBar.Text("True", SKYBLUE);
        } else {
            uiActionBar.Text("False", WHITE);
        }
        uiActionBar.Newline();

        ////////////////////////////////////////////////////////////////////////
        // Life
        uiActionBar.Label("health", labelWidth);
        static STB_TexteditState txtHealth{};
        uiActionBar.TextboxFloat(txtHealth, eLife.health, 80);
        uiActionBar.Text("/");
        static STB_TexteditState txtMaxHealth{};
        uiActionBar.TextboxFloat(txtMaxHealth, eLife.maxHealth, 80);
        uiActionBar.Newline();

        ////////////////////////////////////////////////////////////////////////
        // Physics
        uiActionBar.Label("drag", labelWidth);
        static STB_TexteditState txtDrag{};
        uiActionBar.TextboxFloat(txtDrag, ePhysics.drag, 80);
        uiActionBar.Newline();

        uiActionBar.Label("speed", labelWidth);
        static STB_TexteditState txtSpeed{};
        uiActionBar.TextboxFloat(txtSpeed, ePhysics.speed, 80);
        uiActionBar.Newline();

        uiActionBar.Label("velocity", labelWidth);
        uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeDefault);
        static STB_TexteditState txtVelX{};
        uiActionBar.TextboxFloat(txtVelX, ePhysics.velocity.x, 80);
        uiActionBar.PopStyle();
        uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeDefault);
        static STB_TexteditState txtVelY{};
        uiActionBar.TextboxFloat(txtVelY, ePhysics.velocity.y, 80);
        uiActionBar.PopStyle();
        uiActionBar.Newline();
    }
}
void Editor::DrawUI_SfxFiles(UI &uiActionBar, double now)
{
    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{ "*" };
    uiActionBar.Textbox(txtSearch, filter);
    uiActionBar.Newline();

    for (data::SfxFile &sfxFile : data::pack1.sfxFiles) {
        if (!sfxFile.id) {
            continue;
        }

        Color bgColor = sfxFile.id == state.sfxFiles.selectedSfx ? SKYBLUE : BLUE;
        const char *idStr = sfxFile.path.c_str();
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        if (uiActionBar.Text(idStr, WHITE, bgColor).down) {
            state.sfxFiles.selectedSfx = sfxFile.id;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.sfxFiles.selectedSfx) {
        const int labelWidth = 100;
        data::SfxFile &sfxFile = data::pack1.sfxFiles[state.sfxFiles.selectedSfx];

        uiActionBar.Label("id", labelWidth);
        uiActionBar.Text(TextFormat("%d", sfxFile.id));
        uiActionBar.Newline();

        uiActionBar.Label("path", labelWidth);
        static STB_TexteditState txtPath{};
        uiActionBar.Textbox(txtPath, sfxFile.path);
        uiActionBar.Newline();

        uiActionBar.Label("pitch variance", labelWidth);
        static STB_TexteditState txtPitchVariance{};
        uiActionBar.TextboxFloat(txtPitchVariance, sfxFile.pitch_variance);
        uiActionBar.Newline();
    }
}
void Editor::DrawUI_PackFiles(UI &uiActionBar, double now)
{
    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{ "*" };
    uiActionBar.Textbox(txtSearch, filter);
    uiActionBar.Newline();

    for (data::Pack *packPtr : data::packs) {
        if (!packPtr) {
            continue;
        }

        data::Pack &pack = *packPtr;
        if (!pack.version) {
            continue;
        }

        Color bgColor = packPtr == state.packFiles.selectedPack ? SKYBLUE : BLUE;
        const char *idStr = pack.path.c_str();
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        if (uiActionBar.Text(idStr, WHITE, bgColor).down) {
            state.packFiles.selectedPack = packPtr;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.packFiles.selectedPack) {
        data::Pack &pack = *state.packFiles.selectedPack;

        const int labelWidth = 100;

        uiActionBar.Label("version", labelWidth);
        uiActionBar.Text(TextFormat("%d", pack.version));
        uiActionBar.Newline();

        uiActionBar.Label("tocEntries", labelWidth);
        uiActionBar.Text(TextFormat("%zu", pack.toc.entries.size()));
        uiActionBar.Newline();

        ////////////////////////////////////////////////////////////////////////
        // Scroll panel
        static float panelHeightLastFrame = 0;

        const Vector2 panelTopLeft = uiActionBar.CursorScreen();
        const Rectangle panelRect{ panelTopLeft.x, panelTopLeft.y, 430, GetRenderHeight() - panelTopLeft.y };

        const float scrollOffsetMax = MAX(0, panelHeightLastFrame - panelRect.height + 8);

        static float scrollStacker = 0;
        float &scrollOffset = state.packFiles.scrollOffset;
        float &scrollOffsetTarget = state.packFiles.scrollOffsetTarget;
        float &scrollVelocity = state.packFiles.scrollVelocity;
        if (dlb_CheckCollisionPointRec(GetMousePosition(), panelRect)) {
            const float mouseWheel = io.MouseWheelMove();
            if (mouseWheel) {
                scrollStacker += fabsf(mouseWheel);
                float impulse = 4 * scrollStacker;
                //if (fabsf(mouseWheel) > 1) impulse += powf(10, fabsf(mouseWheel));
                if (mouseWheel < 0) impulse *= -1;
                scrollVelocity += impulse;
                printf("wheel: %f, target: %f\n", mouseWheel, scrollOffsetTarget);
            } else {
                scrollStacker = 0;
            }
        }

        if (scrollVelocity) {
            scrollOffsetTarget -= scrollVelocity;
            scrollVelocity *= 0.95f;
        }

        scrollOffsetTarget = CLAMP(scrollOffsetTarget, 0, scrollOffsetMax);
        scrollOffset = LERP(scrollOffset, scrollOffsetTarget, 0.1);
        scrollOffset = CLAMP(scrollOffset, 0, scrollOffsetMax);

        const float contentStartY = uiActionBar.CursorScreen().y;
        uiActionBar.Space({ 0, -scrollOffset });
        BeginScissorMode(panelRect.x, panelRect.y, panelRect.width, panelRect.height);

        /////// BEGIN CONTENT ///////
        static struct DatTypeFilter {
            bool enabled;
            const char *text;
            Color color;
        } datTypeFilter[data::DAT_TYP_COUNT]{
            { true, "ARR", ColorFromHSV(0 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "GFX", ColorFromHSV(1 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "MUS", ColorFromHSV(2 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "SFX", ColorFromHSV(3 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "FRM", ColorFromHSV(4 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "ANM", ColorFromHSV(5 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "MAT", ColorFromHSV(6 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "TIL", ColorFromHSV(7 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "ENT", ColorFromHSV(8 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "SPT", ColorFromHSV(9 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
        };

        uiActionBar.PushWidth(34);
        for (int i = 0; i < data::DAT_TYP_COUNT; i++) {
            DatTypeFilter &filter = datTypeFilter[i];
            if (uiActionBar.Button(filter.text, filter.enabled, filter.color, ColorBrightness(filter.color, -0.3f)).pressed) {
                if (io.KeyDown(KEY_LEFT_SHIFT)) {
                    filter.enabled = !filter.enabled;
                } else {
                    for (int j = 0; j < data::DAT_TYP_COUNT; j++) {
                        datTypeFilter[j].enabled = false;
                    }
                    filter.enabled = true;
                }
            }
            if (i % 7 == 6 || i == data::DAT_TYP_COUNT - 1) {
                uiActionBar.Newline();
            }
        }
        uiActionBar.PopStyle();

        uiActionBar.PushWidth(400);
        for (data::PackTocEntry &entry : pack.toc.entries) {
            DatTypeFilter &filter = datTypeFilter[entry.dtype];
            if (!filter.enabled) {
                continue;
            }

            //uiActionBar.Label(TextFormat("%s [offset=%d]", DataTypeStr(entry.dtype), entry.offset), labelWidth);
            uiActionBar.Text(DataTypeStr(entry.dtype), WHITE, filter.color);
            uiActionBar.Newline();
        }
        uiActionBar.PopStyle();
        /////// END CONTENT ///////

        EndScissorMode();
        uiActionBar.Space({ 0, scrollOffset });
        const float contentEndY = uiActionBar.CursorScreen().y;
        panelHeightLastFrame = contentEndY - contentStartY;

        const float scrollRatio = panelRect.height / panelHeightLastFrame;
        const float scrollbarWidth = 8;
        const float scrollbarHeight = panelRect.height * scrollRatio;
        const float scrollbarSpace = panelRect.height * (1 - scrollRatio);
        const float scrollPct = scrollOffset / scrollOffsetMax;
        Rectangle scrollbar{
            430 - 2 - scrollbarWidth,
            panelRect.y + scrollbarSpace * scrollPct,
            scrollbarWidth,
            scrollbarHeight
        };
        DrawRectangleRec(scrollbar, LIGHTGRAY);
        ////////////////////////////////////////////////////////////////////////

        DrawRectangleLinesEx(panelRect, 2, BLACK);
    }
}