#include "../common/collision.h"
#include "../common/common.h"
#include "../common/data.h"
#include "../common/entity_db.h"
#include "../common/io.h"
#include "../common/ui/ui.h"
#include "../common/uid.h"
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
        case EditMode_Dialog:    return "Dialog";
        case EditMode_Entities:  return "Entities";
        case EditMode_PackFiles: return "Pack";
        case EditMode_Debug:     return "Debug";
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
        if (io.KeyDown(KEY_LEFT_SHIFT)) {
            dock_left = !dock_left;
            active = true;
        } else {
            active = !active;
        }
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

    auto &map = packs[0].FindById<Tilemap>(map_id);

    // Draw tile collision layer
    if (state.showColliders) {
        map.DrawColliders(camera);
    }
    if (state.showTileEdges) {
        map.DrawEdges();
    }
    if (state.showTileIds) {
        map.DrawTileIds(camera);
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

        TileLayerType layer = state.tiles.layer;
        auto &cursor = state.tiles.cursor;
        auto &map = packs[0].FindById<Tilemap>(map_id);

        Tilemap::Coord coord{};
        map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
        if (editorPlaceTile) {
            Tilemap::Coord selection_center = cursor.SelectionCenter();

            int i = 0;
            for (int y = 0; y < cursor.selection_size.y; y++) {
                for (int x = 0; x < cursor.selection_size.x; x++) {
                    uint16_t tile = cursor.selection_tiles[layer][i];
                    map.SetTry(layer,
                        coord.x - selection_center.x + x,
                        coord.y - selection_center.y + y, tile, now);
                    i++;
                }
            }
        } else if (editorPickTile) {
            if (io.MouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                cursor.pick_start = coord;
            }
            cursor.pick_current = coord;

            Tilemap::Region pick_region = cursor.PickRegion();
            cursor.selection_type = SELECTION_MAP;
            cursor.selection_size.x = pick_region.br.x - pick_region.tl.x + 1;
            cursor.selection_size.y = pick_region.br.y - pick_region.tl.y + 1;

            for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
                cursor.selection_tiles[layer].clear();
                for (int y = pick_region.tl.y; y <= pick_region.br.y; y++) {
                    for (int x = pick_region.tl.x; x <= pick_region.br.x; x++) {
                        uint16_t tile = map.At((TileLayerType)layer, x, y);
                        cursor.selection_tiles[layer].push_back(tile);
                    }
                }
            }
        } else if (editorFillTile) {
            if (cursor.selection_tiles->size()) {
                map.Fill(layer, coord.x, coord.y, cursor.selection_tiles[layer][0], now);
            }
        }

        if (cursor.selection_tiles[layer].size()) {
            Tilemap::Coord selection_center = cursor.SelectionCenter();

            // Find "center" of cursor coord with respect to picked region size
            Vector2 brush_tl{};
            if (editorPickTile) {
                Tilemap::Region pick_region = cursor.PickRegion();
                brush_tl.x = pick_region.tl.x;
                brush_tl.y = pick_region.tl.y;
            } else {
                brush_tl.x = coord.x - selection_center.x;
                brush_tl.y = coord.y - selection_center.y;
            }

            // Draw picked region as brush
            int i = 0;
            for (int y = 0; y < cursor.selection_size.y; y++) {
                for (int x = 0; x < cursor.selection_size.x; x++) {
                    uint16_t tile = cursor.selection_tiles[layer][i];
                    if (tile) {
                        Vector2 brush_tile_pos = brush_tl;
                        brush_tile_pos.x += x;
                        brush_tile_pos.y += y;
                        brush_tile_pos = Vector2Scale(brush_tile_pos, TILE_W);

                        map.DrawTile(tile, brush_tile_pos, 0, Fade(WHITE, 0.8f));
                    }
                    i++;
                }
            }

            // Brush blueness overlay
            Rectangle selectRect{
                brush_tl.x * TILE_W,
                brush_tl.y * TILE_W,
                (float)cursor.selection_size.x * TILE_W,
                (float)cursor.selection_size.y * TILE_W
            };
            DrawRectangleRec(selectRect, Fade(SKYBLUE, 0.2f));
            DrawRectangleLinesEx(selectRect, 2, SKYBLUE);
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
    auto &map = packs[0].FindById<Tilemap>(map_id);
    for (uint16_t aiPathId = 1; aiPathId <= map.paths.size(); aiPathId++) {
        AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint16_t aiPathNodeIndex = aiPath->pathNodeStart; aiPathNodeIndex < aiPath->pathNodeStart + aiPath->pathNodeCount; aiPathNodeIndex++) {
            uint16_t aiPathNodeNextIndex = map.GetNextPathNodeIndex(aiPathId, aiPathNodeIndex);
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
    for (uint16_t aiPathId = 1; aiPathId <= map.paths.size(); aiPathId++) {
        AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint16_t aiPathNodeIndex = aiPath->pathNodeStart; aiPathNodeIndex < aiPath->pathNodeStart + aiPath->pathNodeCount; aiPathNodeIndex++) {
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
                    cursor.dragStartPosition = { aiPathNode->pos.x, aiPathNode->pos.y };
                }
            }

            if (cursor.dragging && cursor.dragPathNodeIndex == aiPathNodeIndex) {
                io.PushScope(IO::IO_EditorDrag);
                if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    io.CaptureMouse();
                    io.CaptureKeyboard();

                    color = aiPathNode->waitFor ? DARKBLUE : MAROON;

                    if (io.KeyPressed(KEY_ESCAPE)) {
                        aiPathNode->pos.x = cursor.dragStartPosition.x;
                        aiPathNode->pos.y = cursor.dragStartPosition.y;
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
            aiPathNode->pos.x = newNodePos.x;
            aiPathNode->pos.y = newNodePos.y;
        }

        io.PopScope();
    }
}

void Editor::DrawEntityOverlays(Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorEntityOverlay);

    Entity *selectedEntity = entityDb->FindEntity(state.entities.selectedId);
    auto &map = packs[0].FindById<Tilemap>(map_id);
    if (selectedEntity && selectedEntity->map_id == map.id) {
        const char *text = TextFormat("[selected]");
        Vector2 tc = GetWorldToScreen2D(selectedEntity->TopCenter(), camera);
        Vector2 textSize = dlb_MeasureTextEx(fntMedium, CSTRLEN(text));
        Vector2 textPos{
            floorf(tc.x - textSize.x / 2.0f),
            tc.y - textSize.y
        };
        dlb_DrawTextShadowEx(fntMedium, CSTRLEN(text), textPos, WHITE);
    }

    if (state.showEntityIds) {
        for (Entity &entity : entityDb->entities) {
            if (entity.map_id == map.id) {
                entityDb->DrawEntityId(entity, camera);
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
    auto &map = packs[0].FindById<Tilemap>(map_id);
    for (Entity &entity : entityDb->entities) {
        if (entity.map_id != map.id) {
            continue;
        }
        assert(entity.id && entity.type);

        // [Debug] Draw entity texture rect
        //DrawRectangleRec(map.EntityRect(i), Fade(SKYBLUE, 0.7f));

        // [Debug] Draw colliders
        if (state.showColliders) {
            if (entity.radius) {
#if 0
                Vector2 topLeft{
                    entity.position.x - entity.radius,
                    entity.position.y - entity.radius
                };
                Vector2 bottomRight{
                    entity.position.x + entity.radius,
                    entity.position.y + entity.radius
                };

                float yMin = CLAMP(floorf(topLeft.y / TILE_W) - 1, 0, 64);
                float yMax = CLAMP(ceilf(bottomRight.y / TILE_W) + 1, 0, 64);
                float xMin = CLAMP(floorf(topLeft.x / TILE_W) - 1, 0, 64);
                float xMax = CLAMP(ceilf(bottomRight.x / TILE_W) + 1, 0, 64);

                Rectangle foo{
                    xMin * TILE_W, yMin * TILE_W, (xMax - xMin) * TILE_W, (yMax - yMin) * TILE_W
                };

                DrawRectangleRec(foo, Fade(YELLOW, 0.5));
#endif
                DrawCircle(
                    entity.position.x, entity.position.y,
                    entity.radius,
                    entity.colliding ? Fade(RED, 0.5) : Fade(LIME, 0.5)
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

struct ScrollPanel {
    Rectangle panelRect        {};
    float scrollOffsetMax      {};
    float scrollOffset         {};
    float scrollOffsetTarget   {};
    float scrollVelocity       {};
    float scrollAccel          {};
    float panelHeightLastFrame {};
};

void BeginScrollPanel(UI &ui, ScrollPanel &scrollPanel, float width)
{
    const Vector2 panelTopLeft = ui.CursorScreen();
    scrollPanel.panelRect = { panelTopLeft.x, panelTopLeft.y, width, GetRenderHeight() - panelTopLeft.y };

    scrollPanel.scrollOffsetMax = MAX(0, scrollPanel.panelHeightLastFrame - scrollPanel.panelRect.height + 8);

    float &scrollOffset = scrollPanel.scrollOffset;
    float &scrollOffsetTarget = scrollPanel.scrollOffsetTarget;
    float &scrollVelocity = scrollPanel.scrollVelocity;
    scrollPanel.scrollAccel = 0;
    if (dlb_CheckCollisionPointRec(GetMousePosition(), scrollPanel.panelRect)) {
        const float mouseWheel = io.MouseWheelMove();
        if (mouseWheel) {
            scrollPanel.scrollAccel += fabsf(mouseWheel);
            float impulse = 4 * scrollPanel.scrollAccel;
            impulse *= impulse;
            if (mouseWheel < 0) impulse *= -1;
            scrollVelocity += impulse;
            //printf("wheel: %f, target: %f\n", mouseWheel, scrollOffsetTarget);
        } else {
            scrollPanel.scrollAccel = 0;
        }
    }

    if (scrollVelocity) {
        scrollOffsetTarget -= scrollVelocity;
        scrollVelocity *= 0.8f;
    }

    scrollOffsetTarget = CLAMP(scrollOffsetTarget, 0, scrollPanel.scrollOffsetMax);
    scrollOffset = LERP(scrollOffset, scrollOffsetTarget, 0.1);
    scrollOffset = CLAMP(scrollOffset, 0, scrollPanel.scrollOffsetMax);

    ui.Space({ 0, -scrollOffset });
    BeginScissorMode(
        scrollPanel.panelRect.x,
        scrollPanel.panelRect.y,
        scrollPanel.panelRect.width,
        scrollPanel.panelRect.height
    );
}
void EndScrollPanel(UI &ui, ScrollPanel &scrollPanel)
{
    EndScissorMode();
    ui.Space({ 0, scrollPanel.scrollOffset });
    const float contentEndY = ui.CursorScreen().y;
    scrollPanel.panelHeightLastFrame = contentEndY - scrollPanel.panelRect.y;

    const float scrollRatio = scrollPanel.panelRect.height / scrollPanel.panelHeightLastFrame;
    const float scrollbarWidth = 8;
    const float scrollbarHeight = scrollPanel.panelRect.height * scrollRatio;
    const float scrollbarSpace = scrollPanel.panelRect.height * (1 - scrollRatio);
    const float scrollPct = scrollPanel.scrollOffset / scrollPanel.scrollOffsetMax;
    Rectangle scrollbar{
        scrollPanel.panelRect.x + scrollPanel.panelRect.width - 2 - scrollbarWidth,
        scrollPanel.panelRect.y + scrollbarSpace * scrollPct,
        scrollbarWidth,
        scrollbarHeight
    };
    DrawRectangleRec(scrollbar, LIGHTGRAY);
    //DrawRectangleLinesEx(scrollPanel.panelRect, 2, WHITE);
}

UIState Editor::DrawUI(double now)
{
    io.PushScope(IO::IO_EditorUI);

    UIState state{};
    if (active) {
        Vector2 pos{};
        if (!dock_left) {
            pos.x = GetRenderWidth() - 430.0f;
        }

        state = DrawUI_ActionBar(pos, now);
        if (state.hover) {
            io.CaptureMouse();
        }
    }

    io.PopScope();
    return state;
}
UIState Editor::DrawUI_ActionBar(Vector2 position, double now)
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
    //DrawRectangleRec(actionBarRect, ColorBrightness(ASESPRITE_BEIGE, -0.1f));
    DrawRectangleRec(actionBarRect, GRAYISH_BLUE);
    DrawRectangleLinesEx(actionBarRect, 2.0f, BLACK);
#endif
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);

    const char *mapFileFilter[1] = { "*.mdesk" };
    static std::string openRequest;
    static std::string saveAsRequest;

    auto &map = packs[0].FindById<Tilemap>(map_id);

#if 0
    UIState openButton = uiActionBar.Button(CSTR("Open"));
    if (openButton.released) {
        std::string filename = "resources/map/" + map.name + ".mdesk";
        std::thread openFileThread([filename, mapFileFilter]{
            const char *openRequestBuf = tinyfd_openFileDialog(
                "Open File",
                filename.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.mdesk)",
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
#endif
    UIState saveButton = uiActionBar.Button(CSTR("Save"));
    if (saveButton.released) {
        std::string path = packs[0].GetPath(PACK_TYPE_TEXT);
        Err err = SavePack(packs[0], PACK_TYPE_TEXT);
        if (err) {
            std::thread errorThread([path, err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", path.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState saveAsButton = uiActionBar.Button(CSTR("Save As"));
    if (saveAsButton.released) {
        std::string path = packs[0].GetPath(PACK_TYPE_TEXT);
        std::thread saveAsThread([path, mapFileFilter]{
            const char *saveAsRequestBuf = tinyfd_saveFileDialog(
                "Save File",
                path.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Pack (*.dat)");
            if (saveAsRequestBuf) saveAsRequest = saveAsRequestBuf;
        });
        saveAsThread.detach();
    }
    if (saveAsRequest.size()) {
        Err err = SavePack(packs[0], PACK_TYPE_TEXT, saveAsRequest);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", saveAsRequest.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        saveAsRequest.clear();
    }

    UIState reloadButton = uiActionBar.Button(CSTR("Reload"));
    if (reloadButton.released) {
#if 0
        // TODO: Probably want to be able to just reload the map, not *everything* right? Hmm..
        Pack reload_pack{ "packs/assets.dat" };
        LoadPack(reload_pack, PACK_TYPE_BINARY);

        // HACK(dlb): What the hell is size() == 2?
        Err err = RN_BAD_FILE_READ;
        if (reload_pack.tile_maps.size() == 2) {
            Tilemap &old_map = packs[0].FindById<Tilemap>(map.id);
            if (&old_map != &packs[0].tile_maps[0]) {
                old_map = reload_pack.tile_maps[1];
                err = RN_SUCCESS;
            }
        }

        if (err) {
            std::string filename = "resources/map/" + map.name + ".mdesk";
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to reload file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
#endif
    }
    uiActionBar.Newline();

    UIState mapPath = uiActionBar.Text(CSTRLEN(GetFileName(map.name.c_str())), WHITE);
    if (mapPath.released) {
        system("explorer maps");
    }
    uiActionBar.Text(CSTRLEN(TextFormat("(v%d)", map.version)));

    uiActionBar.Space({ 0, 8 });
    uiActionBar.Newline();

    UIState showCollidersButton = uiActionBar.Button(CSTR("Collision"), state.showColliders, GRAY, MAROON);
    if (showCollidersButton.released) {
        state.showColliders = !state.showColliders;
    }
    UIState showTileEdgesButton = uiActionBar.Button(CSTR("Edges"), state.showTileEdges, GRAY, MAROON);
    if (showTileEdgesButton.released) {
        state.showTileEdges = !state.showTileEdges;
    }
    UIState showTileIdsButton = uiActionBar.Button(CSTR("Tile IDs"), state.showTileIds, GRAY, LIGHTGRAY);
    if (showTileIdsButton.released) {
        state.showTileIds = !state.showTileIds;
    }
    UIState showEntityIdsButton = uiActionBar.Button(CSTR("Entity IDs"), state.showEntityIds, GRAY, LIGHTGRAY);
    if (showEntityIdsButton.released) {
        state.showEntityIds = !state.showEntityIds;
    }

    uiActionBar.Space({ 0, 8 });
    uiActionBar.Newline();

    for (int i = 0; i < EditMode_Count; i++) {
        if (uiActionBar.Button(CSTRLEN(EditModeStr((EditMode)i)), mode == i, BLUE_DESAT, ColorBrightness(BLUE_DESAT, -0.3f)).pressed) {
            mode = (EditMode)i;
        }
        if (i % 6 == 5) {
            uiActionBar.Newline();
        }
    }

    uiActionBar.Space({ 0, 8 });
    uiActionBar.Newline();

    switch (mode) {
        case EditMode_Maps: {
            DrawUI_MapActions(uiActionBar, now);
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
        case EditMode_Dialog: {
            DrawUI_DialogActions(uiActionBar, now);
            break;
        }
        case EditMode_Entities: {
            DrawUI_EntityActions(uiActionBar, now);
            break;
        }
        case EditMode_PackFiles: {
            DrawUI_PackFiles(uiActionBar, now);
            break;
        }
        case EditMode_Debug: {
            DrawUI_Debug(uiActionBar, now);
            break;
        }
    }

    return uiState;
}
void Editor::DrawUI_MapActions(UI &uiActionBar, double now)
{
    for (const Tilemap &map : packs[0].tile_maps) {
        if (uiActionBar.Button(CSTRLEN(TextFormat("%s", map.name.c_str()))).pressed) {
            map_id = map.id;
        }
        uiActionBar.Newline();
    }
}
void Editor::DrawUI_TileActions(UI &uiActionBar, double now)
{
    const char *mapFileFilter[1] = { "*.png" };
    static const char *openRequest = 0;

#if 0
    const std::string &tilesetPath = rnStringCatalog.GetString(map.textureId);
    uiActionBar.Text(tilesetPath.c_str());
    if (uiActionBar.Button(CSTR("Change tileset"), ColorBrightness(ORANGE, -0.3f)).released) {
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
        Err err = Tilemap::ChangeTileset(*map, openRequest, now);
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
#endif

    auto &map = packs[0].FindById<Tilemap>(map_id);

    static float width = 0;
    static float height = 0;

    const uint32_t ctrlid_width = __COUNTER__;
    const uint32_t ctrlid_height = __COUNTER__;

    if (!UI::IsActiveEditor(ctrlid_width) && !UI::IsActiveEditor(ctrlid_height)) {
        width = map.width;
        height = map.height;
    }

    uiActionBar.Text(CSTR("Size"));
    uiActionBar.PushWidth(100);
    uiActionBar.Textbox(ctrlid_width, width);
    uiActionBar.Textbox(ctrlid_height, height);
    uiActionBar.PopStyle();
    if (uiActionBar.Button(CSTR("Resize!")).pressed) {
        uint16_t newWidth = (uint16_t)width;
        uint16_t newHeight = (uint16_t)height;

        std::array<std::vector<uint16_t>, TILE_LAYER_COUNT> layersNew{};
        for (int i = 0; i < TILE_LAYER_COUNT; i++) {
            layersNew[i].resize(newWidth * newHeight);
            for (uint16_t y = 0; y < map.height && y < newHeight; y++) {
                for (uint16_t x = 0; x < map.width && x < newWidth; x++) {
                    layersNew[i][y * newWidth + x] = map.layers[i][y * map.width + x];
                }
            }
        }

        std::vector<ObjectData> objDataNew{};
        for (ObjectData &obj_data : map.object_data) {
            if (obj_data.x < newWidth && obj_data.y < newHeight) {
                objDataNew.push_back(obj_data);
            }
        }

        map.layers = layersNew;
        map.object_data = objDataNew;
        map.width = (uint16_t)newWidth;
        map.height = (uint16_t)newHeight;
    }
    uiActionBar.Newline();

    uiActionBar.Text(CSTR("Flag"));

    auto &tileEditMode = state.tiles.tileEditMode;
    if (uiActionBar.Button(CSTR("Select"), tileEditMode == TileEditMode_Select, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_Select;
    }
    if (uiActionBar.Button(CSTR("Collision"), tileEditMode == TileEditMode_Collision, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_Collision;
    }
    if (uiActionBar.Button(CSTR("Auto-tile Mask"), tileEditMode == TileEditMode_AutoTileMask, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_AutoTileMask;
    }
    uiActionBar.Newline();

    uiActionBar.Text(CSTR("Layer"));
    if (uiActionBar.Button(CSTR("Ground"), state.tiles.layer == TILE_LAYER_GROUND, GRAY, SKYBLUE).pressed) {
        state.tiles.layer = TILE_LAYER_GROUND;
    }
    if (uiActionBar.Button(CSTR("Object"), state.tiles.layer == TILE_LAYER_OBJECT, GRAY, SKYBLUE).pressed) {
        state.tiles.layer = TILE_LAYER_OBJECT;
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
    // TODO(dlb): Support multi-select (big rectangle?), and figure out where this lives

    auto &map = packs[0].FindById<Tilemap>(map_id);
    auto &tile_defs = packs[0].tile_defs;
    TileLayerType layer = state.tiles.layer;
    auto &cursor = state.tiles.cursor;

    int tiles_x = 6;
    int tiles_y = tile_defs.size() / 6 + (tile_defs.size() % 6 > 0);
    int checker_w = TILE_W * tiles_x;
    int checker_h = TILE_W * tiles_y;

    static Texture checkerTex{};
    if (checkerTex.width != checker_w || checkerTex.height != checker_h) {
        Image checkerImg = GenImageChecked(checker_w, checker_h, 8, 8, GRAY, LIGHTGRAY);
        checkerTex = LoadTextureFromImage(GenImageChecked(checker_w, checker_h, 8, 8, GRAY, LIGHTGRAY));
    }

    UIStyle blackBorderStyle{};
    blackBorderStyle.borderColor = BLACK;
    uiActionBar.PushStyle(blackBorderStyle);

    UIState sheet = uiActionBar.Image(checkerTex);
    Vector2 imgTL{ sheet.contentRect.x, sheet.contentRect.y };

    // Draw tiledefs
    for (size_t i = 0; i < tile_defs.size(); i++) {
        int tile_x = TILE_W * (i % tiles_x);
        int tile_y = TILE_W * (i / tiles_x);
        map.DrawTile(tile_defs[i].id, { imgTL.x + tile_x, imgTL.y + tile_y }, 0);
    }

    uiActionBar.PopStyle();

    // Draw collision overlay on tilesheet if we're in collision editing mode
    switch (state.tiles.tileEditMode) {
        case TileEditMode_Collision: {
            for (size_t i = 0; i < tile_defs.size(); i++) {
                const float tileThird = TILE_W / 3;
                const float tileSixth = tileThird / 2.0f;
                Vector2 cursor{
                    imgTL.x + TILE_W * (i % tiles_x),
                    imgTL.y + TILE_W * (i / tiles_x)
                };

                cursor.x++;
                cursor.y++;

                // TODO: Make this a loop + LUT if we start adding more flags
                if (!tile_defs[i].flags) {
                    DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(MAROON, 0.7f));

                    Vector2 size = dlb_MeasureTextEx(fntSmall, "X", 1, 0);
                    Vector2 pos = cursor;
                    pos.x += tileSixth - size.x / 2.0f;
                    pos.y += tileSixth - size.y / 2.0f;
                    dlb_DrawTextShadowEx(fntSmall, CSTR("X"), pos, WHITE);
                } else {
                    if (tile_defs[i].flags & TileDef::FLAG_SOLID) {
                        DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(DARKGREEN, 0.7f));

                        Vector2 size = dlb_MeasureTextEx(fntSmall, "S", 1, 0);
                        Vector2 pos = cursor;
                        pos.x += tileSixth - size.x / 2.0f;
                        pos.y += tileSixth - size.y / 2.0f;
                        dlb_DrawTextShadowEx(fntSmall, CSTR("S"), pos, WHITE);
                    }
                    cursor.x += tileThird + 1;
                    if (tile_defs[i].flags & TileDef::FLAG_LIQUID) {
                        DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(SKYBLUE, 0.7f));

                        Vector2 size = dlb_MeasureTextEx(fntSmall, "L", 1, 0);
                        Vector2 pos = cursor;
                        pos.x += tileSixth - size.x / 2.0f;
                        pos.y += tileSixth - size.y / 2.0f;
                        dlb_DrawTextShadowEx(fntSmall, CSTR("L"), pos, WHITE);
                    }
                    cursor.x += tileThird + 1;
                }
            }
            break;
        }
        case TileEditMode_AutoTileMask: {
            for (int i = 0; i < tile_defs.size(); i++) {
                const float tileThird = TILE_W / 3;
                float tile_x = TILE_W * (i % tiles_x);
                float tile_y = TILE_W * (i / tiles_x);
                Rectangle tileDefRectScreen{
                    tile_x, tile_y, TILE_W, TILE_W
                };
                tileDefRectScreen.x += imgTL.x + 1;
                tileDefRectScreen.y += imgTL.y + 1;
                tileDefRectScreen.width = tileThird;
                tileDefRectScreen.height = tileThird;

                const Color color = Fade(MAROON, 0.7f);

                Vector2 cursor{};
                uint8_t mask = tile_defs[i].auto_tile_mask;
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

        // Draw hover highlight on tilesheet if mouse hovering a valid tiledef
        const int tile_x = CLAMP((int)mouseRel.x / TILE_W, 0, tiles_x);
        const int tile_y = CLAMP((int)mouseRel.y / TILE_W, 0, tiles_y);

        // If mouse pressed, select tile, or change collision data, depending on mode
        switch (state.tiles.tileEditMode) {
            case TileEditMode_Select: {
                if (!sheet.down) break;

                // Track drag start/end
                if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    cursor.pick_start = { tile_x, tile_y };
                }
                cursor.pick_current = { tile_x, tile_y };

                // Normalize the region so that TL is min and BR is max
                Tilemap::Region pick_region = cursor.PickRegion();
                cursor.selection_type = SELECTION_PALETTE;
                cursor.selection_size.x = pick_region.br.x - pick_region.tl.x + 1;
                cursor.selection_size.y = pick_region.br.y - pick_region.tl.y + 1;

                // Copy tile ids into selection buffer
                for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
                    cursor.selection_tiles[layer].clear();
                    for (int y = pick_region.tl.y; y <= pick_region.br.y; y++) {
                        for (int x = pick_region.tl.x; x <= pick_region.br.x; x++) {
                            int tileIdx = y * tiles_x + x;
                            if (tileIdx < 0 || tileIdx >= tile_defs.size()) {
                                tileIdx = 0;
                            }
                            cursor.selection_tiles[layer].push_back(tile_defs[tileIdx].id);
                        }
                    }
                }
                break;
            }
            case TileEditMode_Collision:
            case TileEditMode_AutoTileMask: {
                if (!sheet.pressed) break;

                int tileIdx = tile_y * tiles_x + tile_x;
                if (tileIdx < 0 || tileIdx > tile_defs.size()) {
                    break;
                }

                const int tileXRemainder = (int)mouseRel.x % TILE_W - 1;
                const int tileYRemainder = (int)mouseRel.y % TILE_W - 1;
                if (tileXRemainder < 0 || tileXRemainder > (TILE_W - 3) ||
                    tileYRemainder < 0 || tileYRemainder > (TILE_W - 3))
                {
                    break;
                }
                const int tileXSegment = tileXRemainder / (TILE_W / 3);
                const int tileYSegment = tileYRemainder / (TILE_W / 3);

                const int tileSegment = tileYSegment * 3 + tileXSegment;

                if (state.tiles.tileEditMode == TileEditMode_Collision) {
                    uint8_t flags = tile_defs[tileIdx].flags;
                    switch (tileSegment) {
                        case 0: flags ^= TileDef::FLAG_SOLID;  break;
                        case 1: flags ^= TileDef::FLAG_LIQUID; break;
                    }
                    tile_defs[tileIdx].flags = (TileDef::Flags)flags;
                } else if (state.tiles.tileEditMode == TileEditMode_AutoTileMask) {
                    //printf("x: %d, y: %d, s: %d\n", tileXSegment, tileYSegment, tileSegment);
                    switch (tileSegment) {
                        case 0: tile_defs[tileIdx].auto_tile_mask ^= 0b10000000; break;
                        case 1: tile_defs[tileIdx].auto_tile_mask ^= 0b01000000; break;
                        case 2: tile_defs[tileIdx].auto_tile_mask ^= 0b00100000; break;
                        case 3: tile_defs[tileIdx].auto_tile_mask ^= 0b00010000; break;
                        case 4: tile_defs[tileIdx].auto_tile_mask ^= 0b00000000; break;
                        case 5: tile_defs[tileIdx].auto_tile_mask ^= 0b00001000; break;
                        case 6: tile_defs[tileIdx].auto_tile_mask ^= 0b00000100; break;
                        case 7: tile_defs[tileIdx].auto_tile_mask ^= 0b00000010; break;
                        case 8: tile_defs[tileIdx].auto_tile_mask ^= 0b00000001; break;
                    }
                }
                break;
            }
        }
    }

    // Draw highlight around currently selected tile(s)
    if (cursor.selection_type == SELECTION_PALETTE) {
        if (cursor.selection_tiles[layer].size()) {
            Tilemap::Region pick_region = cursor.PickRegion();
            Vector2 brush_tl{};
            brush_tl.x = pick_region.tl.x;
            brush_tl.y = pick_region.tl.y;

            // Selected tiles blueness overlay
            Rectangle selectRect{
                imgTL.x + brush_tl.x * TILE_W,
                imgTL.y + brush_tl.y * TILE_W,
                (float)cursor.selection_size.x * TILE_W,
                (float)cursor.selection_size.y * TILE_W
            };
            DrawRectangleRec(selectRect, Fade(SKYBLUE, 0.2f));
            DrawRectangleLinesEx(selectRect, 2, SKYBLUE);
        }
    }

    DrawUI_WangTile(now);
}
void Editor::DrawUI_Wang(UI &uiActionBar, double now)
{
    if (uiActionBar.Button(CSTR("Generate template")).pressed) {
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
    uiWangStyle4x.scale = 2;
    uiActionBar.PushStyle(uiWangStyle4x);

    UIStyle styleSelected{ uiWangStyle4x };
    styleSelected.borderColor = WHITE;

    for (int i = 0; i < wangTileset.ts.num_h_tiles; i++) {
        stbhw_tile *wangTile = wangTileset.ts.h_tiles[i];
        Rectangle srcRect{
            (float)wangTile->x,
            (float)wangTile->y,
            (float)wangTileset.ts.short_side_len * 2,
            (float)wangTileset.ts.short_side_len
        };
        bool stylePushed = false;
        if (i == state.wang.hTex) {
            uiActionBar.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (uiActionBar.Image(wangTileset.thumbnail, srcRect).pressed) {
            state.wang.hTex = state.wang.hTex == i ? -1 : i;
            state.wang.vTex = -1;
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
        if (i == state.wang.vTex) {
            uiActionBar.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (uiActionBar.Image(wangTileset.thumbnail, srcRect).pressed) {
            state.wang.hTex = -1;
            state.wang.vTex = state.wang.vTex == i ? -1 : i;
        }
        if (stylePushed) {
            uiActionBar.PopStyle();
        }
    }
    uiActionBar.Newline();

    if (uiActionBar.Button(CSTR("Export tileset")).pressed) {
        ExportImage(wangTileset.ts.img, wangTileset.filename.c_str());
    }
    uiActionBar.Newline();
    uiActionBar.Image(wangTileset.thumbnail);
    uiActionBar.Newline();

    uiActionBar.PopStyle();

    auto &map = packs[0].FindById<Tilemap>(map_id);

    if (uiActionBar.Button(CSTR("Re-generate Map")).pressed) {
        wangTileset.GenerateMap(map.width, map.height, wangMap);
    }
    uiActionBar.Newline();

    if (uiActionBar.Image(wangMap.colorized).pressed) {
        map.SetFromWangMap(wangMap, now);
    }

    DrawUI_WangTile(now);
}
void Editor::DrawUI_WangTile(double now)
{
    WangTileset &wangTileset = state.wang.wangTileset;
    auto &map = packs[0].FindById<Tilemap>(map_id);
    auto &tile_defs = packs[0].tile_defs;

    Rectangle wangBg{ 434, 4, 560, 560 };

    auto &selected_tiles = state.tiles.cursor.selection_tiles[state.tiles.layer];
    if ((state.wang.hTex >= 0 || state.wang.vTex >= 0) && selected_tiles.size() == 1) {
        const uint16_t selectedTile = selected_tiles[0];
        static double lastUpdatedAt = 0;
        static double lastChangedAt = 0;
        const double updateDelay = 0.02;

        DrawRectangleRec(wangBg, Fade(BLACK, 0.5f));

        UIStyle uiWangTileStyle{};
        uiWangTileStyle.scale = 0.5f;
        uiWangTileStyle.margin = 0;
        uiWangTileStyle.imageBorderThickness = 1;
        UI uiWangTile{ { wangBg.x + 8, wangBg.y + 8 }, uiWangTileStyle };

        uint8_t *imgData = (uint8_t *)wangTileset.ts.img.data;

        if (state.wang.hTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.h_tiles[state.wang.hTex];
            for (int y = 0; y < wangTileset.ts.short_side_len; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len*2; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < tile_defs.size() ? pixel[0] : 0;

                    const GfxFrame &gfx_frame = map.GetTileGfxFrame(selectedTile);
                    const GfxFile &gfx_file = packs[0].FindByName<GfxFile>(gfx_frame.gfx);
                    const Rectangle tileRect = map.TileDefRect(tile);
                    if (uiWangTile.Image(gfx_file.texture, tileRect).down) {
                        pixel[0] = selectedTile; //^ (selectedTile*55);
                        pixel[1] = selectedTile; //^ (selectedTile*55);
                        pixel[2] = selectedTile; //^ (selectedTile*55);
                        lastChangedAt = now;
                    }
                }
                uiWangTile.Newline();
            }
        } else if (state.wang.vTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.v_tiles[state.wang.vTex];
            for (int y = 0; y < wangTileset.ts.short_side_len*2; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < tile_defs.size() ? pixel[0] : 0;

                    const GfxFrame &gfx_frame = map.GetTileGfxFrame(selectedTile);
                    const GfxFile &gfx_file = packs[0].FindByName<GfxFile>(gfx_frame.gfx);
                    const Rectangle tileRect = map.TileDefRect(tile);
                    if (uiWangTile.Image(gfx_file.texture, tileRect).down) {
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

        //////////////////////////////////////////////////////////////////////////////////////////

        if (false) {
            struct fang_tile {
                // the edge or vertex constraints, according to diagram below
                signed char a,b,c,d,e,f;

                // The herringbone wang tile data; it is a bitmap which is either
                // w=2*short_sidelen,h=short_sidelen, or w=short_sidelen,h=2*short_sidelen.
                // it is always RGB, stored row-major, with no padding between rows.
                // (allocate stbhw_tile structure to be large enough for the pixel data)
                //unsigned char pixels[1];
                uint32_t x,y;  // uv coords of top left of tile
            };

            fang_tile h_tile{};
            h_tile.a = 0;
            h_tile.b = 1;
            h_tile.c = 2;
            h_tile.d = 3;
            h_tile.e = 4;
            h_tile.f = 5;

            fang_tile v_tile{};

            struct fang_tileset {
                bool is_corner;
                int num_color[6];  // number of colors for each of 6 edge types or 4 corner types
                int short_side_len;
                std::vector<fang_tile> h_tiles;
                std::vector<fang_tile> v_tiles;
                //int num_h_tiles, max_h_tiles;  // num = size(), max = capacity()
                //int num_v_tiles, max_v_tiles;
            };

            fang_tileset ts{};
            ts.is_corner = false;
            ts.num_color[0] = 1;
            ts.num_color[1] = 1;
            ts.num_color[2] = 1;
            ts.num_color[3] = 1;
            ts.num_color[4] = 1;
            ts.num_color[5] = 1;
            ts.short_side_len = 8;
            ts.h_tiles.push_back(h_tile);
            ts.v_tiles.push_back(v_tile);

            UI uiWangNew{ { wangBg.x + wangBg.width + 8 , wangBg.y }, {} };
            Vector2 cursor = uiWangNew.CursorScreen();
            Rectangle tile1{};
            tile1.x = cursor.x;
            tile1.y = cursor.y;
            tile1.width = TILE_W * 8;
            tile1.height = TILE_W * 8;
            DrawRectangleRec(tile1, DARKGRAY);
        }
    }
}
void Editor::DrawUI_PathActions(UI &uiActionBar, double now)
{
    if (state.pathNodes.cursor.dragging) {
        printf("dragging\n");
    }
    uiActionBar.Text(CSTRLEN(TextFormat(
        "drag: %s, path: %d, node: %d",
        state.pathNodes.cursor.dragging ? "true" : "false",
        state.pathNodes.cursor.dragPathId,
        state.pathNodes.cursor.dragPathNodeIndex
    )));
}
void Editor::DrawUI_DialogActions(UI &uiActionBar, double now)
{
#if 0
    //if (uiActionBar.Button(CSTR("Add"), DARKGREEN).pressed) {
    //    //map.warps.push_back({});
    //}
    //uiActionBar.Newline();

    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{};
    uiActionBar.Textbox(txtSearch, filter);
    uiActionBar.Newline();

    for (Dialog &dialog : dialog_library.dialogs) {
        if (!dialog.id) {
            continue;
        }

        Color bgColor = dialog.id == state.dialog.dialogId ? SKYBLUE : BLUE;
        std::string msg{ dialog.msg };
        if (!StrFilter(msg.c_str(), filter.c_str())) {
            continue;
        }

        if (uiActionBar.Text(msg.c_str(), WHITE, bgColor).down) {
            state.dialog.dialogId = dialog.id;
        }
        uiActionBar.Newline();
    }

    if (state.dialog.dialogId) {
        Dialog *dialog = dialog_library.FindById(state.dialog.dialogId);
        if (dialog) {
            uiActionBar.Text(CSTR("message"));
            uiActionBar.Newline();
            static STB_TexteditState txtMessage{};
            uiActionBar.Textbox(txtMessage, dialog.msg);
            uiActionBar.Newline();

            static STB_TexteditState txtOptionId[SV_MAX_ENTITY_DIALOG_OPTIONS]{};
            for (int i = 0; i < SV_MAX_ENTITY_DIALOG_OPTIONS; i++) {
                uiActionBar.Text(TextFormat("option %d", i));
                uiActionBar.Newline();
                float id = dialog.option_ids[i];
                uiActionBar.Textbox(txtOptionId[i], id, 0, "%.f");
                dialog.option_ids[i] = (DialogId)CLAMP(id, 0, packs[0].dialogs.size() - 1);
                uiActionBar.Newline();
            }
        } else {
            state.dialog.dialogId = 0;
        }
    }

    uiActionBar.PopStyle();
#endif
}
void Editor::DrawUI_EntityActions(UI &uiActionBar, double now)
{
    auto &map = packs[0].FindById<Tilemap>(map_id);
    if (uiActionBar.Button(CSTR("Despawn all"), ColorBrightness(MAROON, -0.3f)).pressed) {
        for (const Entity &entity : entityDb->entities) {
            if (entity.type == Entity::TYP_PLAYER || entity.map_id != map.id) {
                continue;
            }
            assert(entity.id && entity.type);
            entityDb->DespawnEntity(entity.id, now);
        }
    }

    uiActionBar.Newline();
    uiActionBar.Space({ 0, 4 });

    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static std::string filter{ "*" };
    uiActionBar.Textbox(__COUNTER__, filter);
    uiActionBar.Newline();

    for (uint32_t i = 0; i < SV_MAX_ENTITIES; i++) {
        Entity &entity = entityDb->entities[i];
        if (!entity.id || entity.map_id != map.id) {
            continue;
        }

        const char *idStr = TextFormat("[%d] %s", entity.id, EntityTypeStr(entity.type));
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        Color bgColor = entity.id == state.entities.selectedId ? ColorBrightness(ORANGE, -0.3f) : BLUE_DESAT;
        if (uiActionBar.Text(CSTRLEN(idStr), WHITE, bgColor).down) {
            state.entities.selectedId = entity.id;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.entities.selectedId) {
        Entity *entity = entityDb->FindEntity(state.entities.selectedId);
        if (entity) {
            const int labelWidth = 100;

            ////////////////////////////////////////////////////////////////////////
            // Entity
            uiActionBar.Label(CSTR("id"), labelWidth);
            uiActionBar.Text(CSTRLEN(TextFormat("%d", entity->id)));
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("type"), labelWidth);
            uiActionBar.Text(CSTRLEN(EntityTypeStr(entity->type)));
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("position"), labelWidth);
            uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeTextbox);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->position.x);
            uiActionBar.PopStyle();
            uiActionBar.PopStyle();

            uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeTextbox);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->position.y);
            uiActionBar.PopStyle();
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Combat
            uiActionBar.Label(CSTR("attk cooldown"), labelWidth);
            const float attackCooldownLeft = MAX(0, entity->attack_cooldown - (now - entity->last_attacked_at));
            uiActionBar.Text(CSTRLEN(TextFormat("%.3f", attackCooldownLeft)));
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Collision
            uiActionBar.Label(CSTR("radius"), labelWidth);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->radius);
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("colliding"), labelWidth);
            if (entity->colliding) {
                uiActionBar.Text(CSTR("True"), RED);
            } else {
                uiActionBar.Text(CSTR("False"), WHITE);
            }
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("onWarp"), labelWidth);
            if (entity->on_warp) {
                uiActionBar.Text(CSTR("True"), SKYBLUE);
            } else {
                uiActionBar.Text(CSTR("False"), WHITE);
            }
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Life
            uiActionBar.Label(CSTR("health"), labelWidth);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->hp);
            uiActionBar.PopStyle();

            uiActionBar.Text(CSTR("/"));

            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->hp_max);
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Physics
            uiActionBar.Label(CSTR("drag"), labelWidth);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->drag);
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("speed"), labelWidth);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->speed);
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            uiActionBar.Label(CSTR("velocity"), labelWidth);
            uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeTextbox);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->velocity.x);
            uiActionBar.PopStyle();
            uiActionBar.PopStyle();
            uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeTextbox);
            uiActionBar.PushWidth(80);
            uiActionBar.Textbox(__COUNTER__, entity->velocity.y);
            uiActionBar.PopStyle();
            uiActionBar.PopStyle();
            uiActionBar.Newline();
        } else {
            state.entities.selectedId = 0;
        }
    }
}
void Editor::DrawUI_PackFiles(UI &uiActionBar, double now)
{
    UIStyle searchStyle = uiActionBar.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 0, 6);
    uiActionBar.PushStyle(searchStyle);

    static std::string filter{ "*" };
    uiActionBar.Textbox(__COUNTER__, filter);
    uiActionBar.Newline();

    for (Pack &pack : packs) {
        if (!pack.version) {
            continue;
        }

        Color bgColor = &pack == state.packFiles.selectedPack ? SKYBLUE : BLUE_DESAT;
        const char *idStr = pack.name.c_str();
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        if (uiActionBar.Text(CSTRLEN(idStr), WHITE, bgColor).down) {
            state.packFiles.selectedPack = &pack;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.packFiles.selectedPack) {
        Pack &pack = *state.packFiles.selectedPack;

        const int labelWidth = 100;

        uiActionBar.Label(CSTR("version"), labelWidth);
        uiActionBar.Text(CSTRLEN(TextFormat("%d", pack.version)));
        uiActionBar.Newline();

        uiActionBar.Label(CSTR("tocEntries"), labelWidth);
        uiActionBar.Text(CSTRLEN(TextFormat("%zu", pack.toc.entries.size())));
        uiActionBar.Newline();

        static struct DatTypeFilter {
            bool enabled;
            const char *text;
            Color color;
        } datTypeFilter[DAT_TYP_COUNT]{
            { true,  "GFX", ColorFromHSV((int)DAT_TYP_GFX_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MUS", ColorFromHSV((int)DAT_TYP_MUS_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "SFX", ColorFromHSV((int)DAT_TYP_SFX_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "FRM", ColorFromHSV((int)DAT_TYP_GFX_FRAME * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "ANM", ColorFromHSV((int)DAT_TYP_GFX_ANIM  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "SPT", ColorFromHSV((int)DAT_TYP_SPRITE    * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "DEF", ColorFromHSV((int)DAT_TYP_TILE_DEF  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MAT", ColorFromHSV((int)DAT_TYP_TILE_MAT  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MAP", ColorFromHSV((int)DAT_TYP_TILE_MAP  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "ENT", ColorFromHSV((int)DAT_TYP_ENTITY    * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
        };

        uiActionBar.PushWidth(34);
        for (int i = 0; i < DAT_TYP_COUNT; i++) {
            DatTypeFilter &filter = datTypeFilter[i];
            if (uiActionBar.Button(CSTRLEN(filter.text ? filter.text : "???"), filter.enabled, DARKGRAY, filter.color).pressed) {
                if (io.KeyDown(KEY_LEFT_SHIFT)) {
                    filter.enabled = !filter.enabled;
                } else {
                    for (int j = 0; j < DAT_TYP_COUNT; j++) {
                        datTypeFilter[j].enabled = false;
                    }
                    filter.enabled = true;
                }
            }
            if ((i % 7 == 7 - 1) || i == DAT_TYP_COUNT - 1) {
                uiActionBar.Newline();
            }
        }
        uiActionBar.PopStyle();

        uiActionBar.Space({ 0, 4 });

        static ScrollPanel scrollPanel{};
        BeginScrollPanel(uiActionBar, scrollPanel, 430);

        // Defer changing selection until after the loop has rendered every item
        int newSelectedOffset = 0;

        for (PackTocEntry &entry : pack.toc.entries) {
            DatTypeFilter &filter = datTypeFilter[entry.dtype];
            if (!filter.enabled) {
                continue;
            }

            bool selected = state.packFiles.selectedPackEntryOffset == entry.offset;

            const char *desc = DataTypeStr(entry.dtype);

            switch (entry.dtype) {
                case DAT_TYP_GFX_FILE:
                {
                    GfxFile &gfx_file = pack.gfx_files[entry.index];
                    desc = gfx_file.name.c_str();
                    break;
                }
                case DAT_TYP_MUS_FILE:
                {
                    MusFile &mus_file = pack.mus_files[entry.index];
                    desc = mus_file.name.c_str();
                    break;
                }
                case DAT_TYP_SFX_FILE:
                {
                    SfxFile &sfx_file = pack.sfx_files[entry.index];
                    desc = sfx_file.name.c_str();
                    break;
                }
                case DAT_TYP_GFX_FRAME:
                {
                    GfxFrame &gfx_frame = pack.gfx_frames[entry.index];
                    desc = gfx_frame.name.c_str();
                    break;
                }
                case DAT_TYP_GFX_ANIM:
                {
                    GfxAnim &gfx_anim = pack.gfx_anims[entry.index];
                    desc = gfx_anim.name.c_str();
                    break;
                }
                case DAT_TYP_SPRITE:
                {
                    Sprite &sprite = pack.sprites[entry.index];
                    desc = sprite.name.c_str();
                    break;
                }
                case DAT_TYP_TILE_DEF:
                {
                    TileDef &tile_def = pack.tile_defs[entry.index];
                    desc = tile_def.name.c_str();
                    break;
                }
                case DAT_TYP_TILE_MAT:
                {
                    TileMat &tile_mat = pack.tile_mats[entry.index];
                    desc = tile_mat.name.c_str();
                    break;
                }
                case DAT_TYP_TILE_MAP:
                {
                    Tilemap &tile_map = pack.tile_maps[entry.index];
                    desc = tile_map.name.c_str();
                    break;
                }
                case DAT_TYP_ENTITY:
                {
                    Entity &entity = pack.entities[entry.index];
                    desc = EntityTypeStr(entity.type);
                    break;
                }
            }

            uiActionBar.PushWidth(400);
            uiActionBar.PushMargin({ 1, 3, 1, 3 });

            const char *text = TextFormat("[%s] %s", selected ? "-" : "+", desc);
            if (uiActionBar.Text(CSTRLEN(text), WHITE, ColorBrightness(filter.color, -0.2f)).pressed) {
                if (!selected) {
                    newSelectedOffset = entry.offset;
                } else {
                    state.packFiles.selectedPackEntryOffset = 0;
                }
            }

            uiActionBar.PopStyle();
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            if (selected) {
                #define HAQ_UI_FIELD(c_type, c_name, c_init, flags, condition, parent) \
                    if (condition) { \
                        uiActionBar.HAQField(__COUNTER__, #c_name, parent.c_name, (flags), labelWidth); \
                    }

                #define HAQ_UI(hqt, parent) \
                    hqt(HAQ_UI_FIELD, parent)

                switch (entry.dtype) {
                    case DAT_TYP_GFX_FILE:
                    {
                        GfxFile &gfx_file = pack.gfx_files[entry.index];
                        HAQ_UI(HQT_GFX_FILE_FIELDS, gfx_file);
                        break;
                    }
                    case DAT_TYP_MUS_FILE:
                    {
                        MusFile &mus_file = pack.mus_files[entry.index];
                        HAQ_UI(HQT_MUS_FILE_FIELDS, mus_file);
                        break;
                    }
                    case DAT_TYP_SFX_FILE:
                    {
                        SfxFile &sfx_file = pack.sfx_files[entry.index];
                        HAQ_UI(HQT_SFX_FILE_FIELDS, sfx_file);

                        if (!IsSoundPlaying(sfx_file.name)) {
                            if (uiActionBar.Button(CSTR("Play"), ColorBrightness(DARKGREEN, -0.3f)).pressed) {
                                PlaySound(sfx_file.name, 0);
                            }
                        } else {
                            if (uiActionBar.Button(CSTR("Stop"), ColorBrightness(MAROON, -0.3f)).pressed) {
                                StopSound(sfx_file.name);
                            }
                        }
                        uiActionBar.Newline();
                        break;
                    }
                    case DAT_TYP_GFX_FRAME:
                    {
                        const float labelWidth = 40;
                        GfxFrame &gfx_frame = pack.gfx_frames[entry.index];
                        HAQ_UI(HQT_GFX_FRAME_FIELDS, gfx_frame);
                        break;
                    }
                    case DAT_TYP_GFX_ANIM:
                    {
                        GfxAnim &gfx_anim = pack.gfx_anims[entry.index];
                        HAQ_UI(HQT_GFX_ANIM_FIELDS, gfx_anim);
#if 0
                        uiActionBar.Label(CSTR("id"), labelWidth);
                        uiActionBar.Text(CSTRS(gfxAnim.id));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("sound"), labelWidth);
                        uiActionBar.Text(CSTRS(gfxAnim.sound));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("frame_delay"), labelWidth);
                        uiActionBar.Text(CSTRLEN(TextFormat("%u", gfxAnim.frame_delay)));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("frames"), labelWidth);
                        uiActionBar.Newline();

                        for (int i = 0; i < gfxAnim.frames.size(); i++) {
                            uiActionBar.Text(CSTRLEN(TextFormat("[%d] %s", i, gfxAnim.frames[i].c_str())));
                            uiActionBar.Newline();
                        }
#endif
                        break;
                    }
                    case DAT_TYP_SPRITE:
                    {
                        const float labelWidth = 40.0f;
                        Sprite &sprite = pack.sprites[entry.index];
#if 1
                        HAQ_UI(HQT_SPRITE_FIELDS, sprite);
#else
                        uiActionBar.Label(CSTR("name"), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.name));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("N "), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_N]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("E "), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_E]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("S "), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_S]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("S "), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_W]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("NE"), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_SE]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("SE"), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_SE]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("SW"), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_SW]));
                        uiActionBar.Newline();

                        uiActionBar.Label(CSTR("NW"), labelWidth);
                        uiActionBar.Text(CSTRS(sprite.anims[DIR_NW]));
                        uiActionBar.Newline();
#endif
                        break;
                    }
                    case DAT_TYP_TILE_DEF:
                    {
                        TileDef &tile_def = pack.tile_defs[entry.index];
                        HAQ_UI(HQT_TILE_DEF_FIELDS, tile_def);
                        break;
                    }
                    case DAT_TYP_TILE_MAT:
                    {
                        TileMat &tile_mat = pack.tile_mats[entry.index];
                        HAQ_UI(HQT_TILE_MAT_FIELDS, tile_mat);
                        break;
                    }
                    case DAT_TYP_TILE_MAP:
                    {
                        Tilemap &tile_map = pack.tile_maps[entry.index];
                        HAQ_UI(HQT_TILE_MAP_FIELDS, tile_map);
                        break;
                    }
                    case DAT_TYP_ENTITY:
                    {
                        uiActionBar.Text(CSTR("TODO"));
                        uiActionBar.Newline();
                        break;
                    }
                }

                #undef HAQ_UI_TYPE
                #undef HAQ_UI_FIELD
                #undef HAQ_UI
            }
        }

        if (newSelectedOffset) {
            state.packFiles.selectedPackEntryOffset = newSelectedOffset;
        }

        EndScrollPanel(uiActionBar, scrollPanel);
    }
}
void Editor::DrawUI_Debug(UI &uiActionBar, double now)
{
    static UID uid = NextUID();
    uiActionBar.Text(CSTRLEN(TextFormat("%.*s", 20, uid.bytes)));
    if (true) {// || uiActionBar.Button(CSTR("NextUID")).pressed) {
        uid = NextUID();
    }
    uiActionBar.Newline();

#if 0
    uiActionBar.Text(CSTR("^^^ ignore all this stuff ^^^"));
    uiActionBar.Space({ 0, 50 });
    uiActionBar.Newline();

    struct Element {
        bool built;
        bool selected;
    };

    const Element elements_default[]{
        { true, false },
        { true, false },
        { false, false },
        { false, false },
    };

    static Element elements []{
        { true, false },
        { true, false },
        { false, false },
        { false, false },
    };

    if (uiActionBar.Button(CSTR("Reset")).pressed) {
        for (int i = 0; i < ARRAY_SIZE(elements); i++) {
            elements[i] = elements_default[i];
        }
    }
    uiActionBar.Newline();

    const bool *built = 0;
    bool multi = false;
    int selected_count = 0;

    uiActionBar.Text(CSTR("_____________________________________________"), GRAY);
    uiActionBar.Newline();
    uiActionBar.Text(CSTR("Elements:"), LIGHTGRAY);
    uiActionBar.Newline();
    for (int i = 0; i < ARRAY_SIZE(elements); i++) {
        uiActionBar.Text(elements[i].selected ? "[x]" : "[   ]");
        Color bgColor = elements[i].built ? DARKGRAY : LIGHTGRAY;
        Color fgColor = elements[i].built ? WHITE : BLACK;
        uiActionBar.PushFgColor(fgColor);
        if (uiActionBar.Button(
                TextFormat("Element #%d - %s", i, elements[i].built ? "Built" : "Not Built"),
                elements[i].selected,
                bgColor,
                ColorBrightness(bgColor, -0.3f)
            )
            .pressed)
        {
            elements[i].selected = !elements[i].selected;
        }
        if (elements[i].selected) {
            if (!built) {
                built = &elements[i].built;
            } else if (elements[i].built != *built) {
                multi = true;
            }
            selected_count++;
        }
        uiActionBar.PopStyle();
        uiActionBar.Newline();
    }

    if (multi) {
        uiActionBar.Button(CSTR("Various Statuses"), true, PURPLE, PURPLE);
    }
    uiActionBar.Newline();

    uiActionBar.Text(CSTR("_____________________________________________"), GRAY);
    uiActionBar.Newline();
    uiActionBar.Text(CSTR("Element Inspector:"), LIGHTGRAY);
    uiActionBar.Newline();
    if (selected_count) {
        //uiActionBar.PushFgColor(BLACK);
        if (uiActionBar.Button(multi ? "Mark all Not Built" : "Not Built", !multi && !*built, GRAY, ColorBrightness(ORANGE, -0.3f)).pressed) {
            for (int i = 0; i < ARRAY_SIZE(elements); i++) {
                if (elements[i].selected) elements[i].built = false;
            }
        }
        //uiActionBar.PopStyle();
        if (uiActionBar.Button(multi ? "Mark all Built" : "Built", !multi && *built, GRAY, ColorBrightness(ORANGE, -0.3f)).pressed) {
            for (int i = 0; i < ARRAY_SIZE(elements); i++) {
                if (elements[i].selected) elements[i].built = true;
            }
        }
    } else {
        uiActionBar.Text(CSTR("Select some elements to see their status."));
    }
    uiActionBar.Newline();
    uiActionBar.Text(CSTR("_____________________________________________"), GRAY);
    uiActionBar.Newline();
#endif
}