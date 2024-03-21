﻿#include "../common/collision.h"
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
        case EditMode_Objects:   return "Objects";
        case EditMode_Wang:      return "Wang";
        case EditMode_Paths:     return "Paths";
        case EditMode_Dialog:    return "Dialog";
        case EditMode_Entities:  return "Entities";
        case EditMode_PackFiles: return "Pack";
        case EditMode_Debug:     return "Debug";
        default: assert(!"unhandled mode"); return "<null>";
    }
}

Err Editor::Init(void)
{
    Err err = RN_SUCCESS;
    err = state.wang.wangTileset.Load("resources/wang/tileset2x2.png");
    return err;
}

void Editor::HandleInput(Camera2D &camera, double now, double dt)
{
    this->now = now;
    this->dt = dt;

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

    if (mode == EditMode_Tiles && state.showFloodDebug) {
        if (io.KeyPressed(KEY_LEFT, true)) {
            if (state.tiles.lastFloodData.step > 0) {
                state.tiles.lastFloodData.step--;
            }
        }
        if (io.KeyPressed(KEY_RIGHT, true)) {
            if (state.tiles.lastFloodData.step < state.tiles.lastFloodData.change_list.size() - 1) {
                state.tiles.lastFloodData.step++;
            }
        }
    }

    io.PopScope();
}

void Editor::DrawGroundOverlays(Camera2D &camera)
{
    io.PushScope(IO::IO_EditorGroundOverlay);

    auto &map = pack_maps.FindById<Tilemap>(map_id);

    // Draw tile collision layer
    if (state.showColliders) {
        map.DrawColliders(camera);
    }
    if (state.showTileEdges) {
        map.DrawEdges();
    }
    if (state.showANYA) {
        map.DrawIntervals(camera);

#if 1
        // DEBUG: ClosestPointLineSegmentToRay
        // In ANYA edit mode, hold left mouse and press QWER to move the lines.
        // Red: Segment
        // Green: Line
        // Yellow: Closest point on segment to line (if segment would eventually intersect the line)
        static Vector2 segmentA {}; //  0 * TILE_W, 43 * TILE_W };
        static Vector2 segmentB {}; // 64 * TILE_W, 43 * TILE_W };
        static Vector2 lineA    {}; // 25 * TILE_W, 45 * TILE_W };
        static Vector2 lineB    {}; // 25 * TILE_W, 44 * TILE_W };

        if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (io.KeyDown(KEY_Q)) {
                segmentA = Vector2Floor(GetScreenToWorld2D(GetMousePosition(), camera));
            } else if (io.KeyDown(KEY_W)) {
                segmentB = Vector2Floor(GetScreenToWorld2D(GetMousePosition(), camera));
            } else if (io.KeyDown(KEY_E)) {
                lineA = Vector2Floor(GetScreenToWorld2D(GetMousePosition(), camera));
            } else if (io.KeyDown(KEY_R)) {
                lineB = Vector2Floor(GetScreenToWorld2D(GetMousePosition(), camera));
            }
        }

        Vector2 closest{};
        bool intersects = ClosestPointLineSegmentToRay(segmentA, segmentB, lineA, lineB, closest);
        DrawLineEx(segmentA, segmentB, 4, RED);
        DrawLineEx(lineA, lineB, 4, GREEN);
        if (intersects) {
            DrawCircleV(closest, 8, YELLOW);
        }

        Vector2 segmentAB = Vector2Subtract(segmentB, segmentA);
        const float rulerLen = Vector2Length(segmentAB);
        const char *rulerTxt = TextFormat("%.2f", rulerLen / TILE_W);
        Vector2 rulerTxtSize = dlb_MeasureTextShadowEx(fntMedium, CSTRLEN(rulerTxt), 0);
        Vector2 rulerTxtOffset = Vector2Floor({ -rulerTxtSize.x * 0.5f, -rulerTxtSize.y * 0.5f });

        Vector2 segmentABHalf = Vector2Floor(Vector2Add(segmentA, Vector2Scale(segmentAB, 0.5f)));
        dlb_DrawTextShadowEx(fntMedium, CSTRLEN(rulerTxt), Vector2Add(segmentABHalf, rulerTxtOffset), WHITE);
#endif
    }
    if (state.showTileIds) {
        map.DrawTileIds(camera, state.tiles.layer);
    }
    if (state.showObjects) {
        map.DrawObjects(camera);
    }
    if (state.showFloodDebug) {
        map.DrawFloodDebug(state.tiles.lastFloodData);
    }

    if (active) {
        switch (mode) {
            case EditMode_Tiles:   DrawGroundOverlay_Tiles   (camera); break;
            case EditMode_Objects: DrawGroundOverlay_Objects (camera); break;
            case EditMode_Wang:    DrawGroundOverlay_Wang    (camera); break;
            case EditMode_Paths:   DrawGroundOverlay_Paths   (camera); break;
        }
    }

    io.PopScope();
}
void Editor::DrawGroundOverlay_Tiles(Camera2D &camera)
{
    if (!io.MouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPlaceTile = io.MouseButtonDown(MOUSE_BUTTON_LEFT);
        const bool editorPickTile = io.MouseButtonDown(MOUSE_BUTTON_MIDDLE);
        const bool editorFillTile = io.KeyPressed(KEY_F);

        TileLayerType layer = state.tiles.layer;
        auto &cursor = state.tiles.cursor;
        auto &map = pack_maps.FindById<Tilemap>(map_id);

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
                if (io.KeyDown(KEY_LEFT_SHIFT)) {
                    state.tiles.lastFloodData = map.FloodDebug(layer, coord.x, coord.y, cursor.selection_tiles[layer][0]);
                    state.showFloodDebug = true;
                } else {
                    map.Flood(layer, coord.x, coord.y, cursor.selection_tiles[layer][0], now);
                }
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

                        DrawTile(tile, brush_tile_pos, 0, Fade(WHITE, 0.8f));
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
void Editor::DrawGroundOverlay_Objects(Camera2D &camera)
{
    if (!io.MouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPickObject = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);

        TileLayerType layer = state.tiles.layer;
        auto &map = pack_maps.FindById<Tilemap>(map_id);

        Tilemap::Coord coord{};
        map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
        if (editorPickObject) {
            state.objects.selected_coord = coord;
        }

        // Hovered obj yellow highlight
        Vector2 brush_tl{
            (float)coord.x * TILE_W,
            (float)coord.y * TILE_W,
        };
        Rectangle selectRect{
            brush_tl.x,
            brush_tl.y,
            TILE_W,
            TILE_W
        };
        DrawRectangleRec(selectRect, Fade(YELLOW, 0.1f));
        DrawRectangleLinesEx(selectRect, 2, Fade(YELLOW, 0.2f));
    }

    // Selected obj yellow highlight
    Vector2 brush_tl{
        (float)state.objects.selected_coord.x * TILE_W,
        (float)state.objects.selected_coord.y * TILE_W,
    };
    Rectangle selectRect{
        brush_tl.x,
        brush_tl.y,
        TILE_W,
        TILE_W
    };
    DrawRectangleRec(selectRect, Fade(YELLOW, 0.2f));
    DrawRectangleLinesEx(selectRect, 2, YELLOW);
}
void Editor::DrawGroundOverlay_Wang(Camera2D &camera)
{
}
void Editor::DrawGroundOverlay_Paths(Camera2D &camera)
{
    auto &cursor = state.pathNodes.cursor;
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

    // Draw path edges
    auto &map = pack_maps.FindById<Tilemap>(map_id);
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

void Editor::DrawEntityOverlays(Camera2D &camera)
{
    io.PushScope(IO::IO_EditorEntityOverlay);

    Entity *selectedEntity = entityDb->FindEntity(state.entities.selectedId);
    auto &map = pack_maps.FindById<Tilemap>(map_id);
    if (selectedEntity && selectedEntity->map_id == map.id) {
        const char *text = TextFormat("[selected]");
        Vector2 tc = GetWorldToScreen2D(selectedEntity->TopCenter(), camera);
        Vector2 textSize = dlb_MeasureTextShadowEx(fntMedium, CSTRLEN(text));
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
                DrawEntityOverlay_Collision(camera);
                break;
            }
        }
    }

    io.PopScope();
}
void Editor::DrawEntityOverlay_Collision(Camera2D &camera)
{
    auto &map = pack_maps.FindById<Tilemap>(map_id);
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

void Editor::CenterCameraOnMap(Camera2D &camera)
{
    // Reset camera when map changes
    auto &new_editor_map = pack_maps.FindById<Tilemap>(map_id);

    Vector2 viewportSize{
        (g_RenderSize.x - width),
        g_RenderSize.y
    };
    Vector2 mapSize{
        (float)new_editor_map.width * TILE_W,
        (float)new_editor_map.height * TILE_W,
    };

    camera.offset = {
        viewportSize.x / 2.0f,
        viewportSize.y / 2.0f
    };
    camera.zoom = MIN(
        viewportSize.x / mapSize.x,
        viewportSize.y / mapSize.y
    );
    camera.zoom = CLAMP(camera.zoom, 0.1f, 2.0f);
    camera.target = {
        mapSize.x / 2.0f,
        mapSize.y / 2.0f
    };
}

void DrawRectangleRectOffset(const Rectangle &rect, Vector2 &offset, Color color)
{
    Rectangle offsetRect = rect;
    offsetRect.x += offset.x;
    offsetRect.y += offset.y;
    DrawRectangleRec(offsetRect, Fade(color, 0.7f));
    DrawRectangleLinesEx(offsetRect, 1, color);
}

void Editor::BeginSearchBox(UI &ui, SearchBox &searchBox)
{
    ui.TextboxWithDefault(hash_combine(&searchBox), searchBox.filter, searchBox.placeholder);
}

bool FilterGfxFrameByGfxFile(Editor &editor, Pack &pack, const void *dat)
{
    GfxFrame *gfx_frame = (GfxFrame *)dat;
    uint16_t gfx_file_id = editor.state.selections.byType[GfxFile::dtype];
    return gfx_frame->gfx == pack.FindById<GfxFile>(gfx_file_id).name;
}

template <typename T>
void Editor::PackSearchBox(UI &ui, Pack &pack, SearchBox &searchBox, SearchFilterFn *customFilter)
{
    auto &vec = *(std::vector<T> *)pack.GetPool(T::dtype);

    ui.PushWidth(width - 112);
    BeginSearchBox(ui, searchBox);
    ui.PopStyle();

    if (ui.Button("Clear", searchBox.filter.size() ? ColorBrightness(MAROON, -0.3f) : GRAY).pressed) {
        searchBox.filter = "";
    }
    UIState add = ui.Button("Add", searchBox.hasMatches ? GRAY : LIME);
    if (add.pressed && !searchBox.hasMatches) {
        T dat{};
        dat.name = searchBox.filter;
        pack.Add<T>(dat);
    }
    searchBox.hasMatches = false;
    ui.Newline();

    ui.PushSize({ width - 12, 120 });
    ui.PushPadding({});
    ui.BeginScrollPanel(searchBox.panel, IO::IO_ScrollPanelInner);
    ui.PopStyle();
    ui.PopStyle();

    ui.PushSize({ width - 18, 0 });
    ui.PushMargin({});
    ui.PushPadding({ 0, 0, 0, 4 });

    int i = 0;

    for (T &dat : vec) {
        i++;
        if (i == 325) {
            printf("");
        }
        const char *idStr = dat.name.c_str();
        if (!StrFilter(idStr, searchBox.filter.c_str())) {
            continue;
        }
        if (customFilter && !(*customFilter)(*this, pack, &dat)) {
            continue;
        }
        searchBox.hasMatches = true;

        Color bgColor = dat.id == state.selections.byType[T::dtype] ? SKYBLUE : BLUE_DESAT;
        if (ui.Text(idStr, WHITE, bgColor).down) {
            state.selections.byType[T::dtype] = dat.id;
        }
        ui.Newline();
    }

    ui.PopStyle();
    ui.PopStyle();
    ui.PopStyle();

    ui.EndScrollPanel(searchBox.panel);
    ui.Newline();
}

void Editor::DrawUI(Camera2D &camera)
{
    io.PushScope(IO::IO_EditorUI);

    if (active) {
        DrawUI_ActionBar();
        if (showGfxFrameEditor) DrawUI_GfxFrameEditor();
        if (showGfxAnimEditor)  DrawUI_GfxAnimEditor();
        if (showSpriteEditor)   DrawUI_SpriteEditor();
        if (showTileDefEditor)  DrawUI_TileDefEditor();

        UI::DrawTooltips();

        static uint16_t editor_map_id = map_id;
        if (map_id != editor_map_id) {
            CenterCameraOnMap(camera);
            editor_map_id = map_id;
        }

        showTileDefEditorDirty = false;
        showGfxAnimEditorDirty = false;
    }

    io.PopScope();
}
void Editor::DrawUI_ActionBar(void)
{
    Vector2 uiPosition{};
    if (dock_left) {
        uiPosition = { 0, 0 };
    } else {
        uiPosition = { g_RenderSize.x - width, 0 };
    }
    Vector2 uiSize{ width, g_RenderSize.y };

    UIStyle uiStyle{};
    UI ui{ uiPosition, uiSize, uiStyle };

    UIState uiState{};
    const Rectangle actionBarRect{ uiPosition.x, uiPosition.y, width, g_RenderSize.y };
#if 0
    DrawRectangleRounded(actionBarRect, 0.15f, 8, ASESPRITE_BEIGE);
    DrawRectangleRoundedLines(actionBarRect, 0.15f, 8, 2.0f, BLACK);
#else
    //DrawRectangleRec(actionBarRect, ColorBrightness(ASESPRITE_BEIGE, -0.1f));
    DrawRectangleRec(actionBarRect, uiStyle.bgColor[UI_CtrlTypePanel]);
    DrawRectangleLinesEx(actionBarRect, uiStyle.borderWidth[UI_CtrlTypePanel], uiStyle.borderColor[UI_CtrlTypePanel]);
#endif
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);
    if (uiState.hover) {
        io.CaptureMouse();
    }

    const char *mapFileFilter[1] = { "*.mdesk" };
    static std::string openRequest;
    static std::string saveAsRequest;

    auto &map = pack_maps.FindById<Tilemap>(map_id);

#if 0
    UIState openButton = ui.Button("Open");
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
            const std::string &msg = TextFormat("Failed to load file %s.\n", openRequest);
            std::thread errorThread([msg]{ tinyfd_messageBox("Error", msg.data(), "ok", "error", 1); });
            errorThread.detach();
        }
        openRequest.clear();
    }
#endif
    UIState saveButton = ui.Button("Save");
    if (saveButton.released) {
        {
            std::string path = pack_assets.GetPath(PACK_TYPE_TEXT);
            Err err = SavePack(pack_assets, PACK_TYPE_TEXT);
            if (err) {
                const std::string &msg = TextFormat("Failed to save file %s. %s\n", path.c_str(), ErrStr(err));
                std::thread errorThread([msg]{ tinyfd_messageBox("Error", msg.data(), "ok", "error", 1); });
                errorThread.detach();
            }
        }
        {
            std::string path = pack_maps.GetPath(PACK_TYPE_TEXT);
            Err err = SavePack(pack_maps, PACK_TYPE_TEXT);
            if (err) {
                const std::string &msg = TextFormat("Failed to save file %s. %s\n", path.c_str(), ErrStr(err));
                std::thread errorThread([msg]{ tinyfd_messageBox("Error", msg.data(), "ok", "error", 1); });
                errorThread.detach();
            }
        }
    }

    UIState saveAsButton = ui.Button("Save As", GRAY);
#if 0
    if (saveAsButton.released) {
        {
            std::string path = pack_assets.GetPath(PACK_TYPE_TEXT);
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
        {
            std::string path = pack_maps.GetPath(PACK_TYPE_TEXT);
            std::thread saveAsThread([path, mapFileFilter]{
                const char *saveAsRequestBuf = tinyfd_saveFileDialog(
                    "Save File",
                    path.c_str(),
                    ARRAY_SIZE(mapFileFilter),
                    mapFileFilter,
                    "RayNet Pack (*.dat)");
                // HACK(dlb): This doesn't work we would need a temp buffer per pack name.
                assert(!"NOPE"); if (saveAsRequestBuf) saveAsRequest = saveAsRequestBuf;
            });
            saveAsThread.detach();
        }
    }
    if (saveAsRequest.size()) {
        Err err = SavePack(pack_assets, PACK_TYPE_TEXT, saveAsRequest);
        if (err) {
            const std::string &msg = TextFormat("Failed to save file %s. %s\n", saveAsRequest.c_str(), ErrStr(err));
            std::thread errorThread([msg]{ tinyfd_messageBox("Error", msg.data(), "ok", "error", 1); });
            errorThread.detach();
        }
        saveAsRequest.clear();
    }
#endif

    UIState reloadButton = ui.Button("Reload", GRAY);
#if 0
    if (reloadButton.released) {
        // TODO: Probably want to be able to just reload the map, not *everything* right? Hmm..
        Pack reload_pack{ "packs/assets.dat" };
        LoadPack(reload_pack, PACK_TYPE_BINARY);

        // HACK(dlb): What the hell is size() == 2?
        Err err = RN_BAD_FILE_READ;
        if (reload_pack.tile_maps.size() == 2) {
            Tilemap &old_map = pack_maps.FindById<Tilemap>(map.id);
            if (&old_map != &pack_maps.tile_maps[0]) {
                old_map = reload_pack.tile_maps[1];
                err = RN_SUCCESS;
            }
        }

        if (err) {
            std::string filename = "resources/map/" + map.name + ".mdesk";
            const std::string &msg = TextFormat("Failed to reload file %s. %s\n", filename.c_str(), ErrStr(err));
            std::thread errorThread([msg]{ tinyfd_messageBox("Error", msg.data(), "ok", "error", 1); });
            errorThread.detach();
        }
    }
#endif
    ui.Newline();

    UIState mapPath = ui.Text(GetFileName(map.name.c_str()), WHITE);
    if (mapPath.released) {
        system("explorer maps");
    }
    ui.Text(TextFormat("(v%d)", map.version));

    ui.Space({ 0, 8 });
    ui.Newline();

    UIState showCollidersButton = ui.ToggleButton("Collision", state.showColliders, GRAY, MAROON);
    if (showCollidersButton.released) {
        state.showColliders = !state.showColliders;
    }
    UIState showTileEdgesButton = ui.ToggleButton(TextFormat("Edges (%u)", map.edges.size()), state.showTileEdges, GRAY, MAROON);
    if (showTileEdgesButton.released) {
        state.showTileEdges = !state.showTileEdges;
    }
    UIState showANYAButton = ui.ToggleButton(TextFormat("ANYA (%u)", map.intervals.size()), state.showANYA, GRAY, MAROON);
    if (showANYAButton.released) {
        state.showANYA = !state.showANYA;
    }
    UIState showTileIdsButton = ui.ToggleButton("Tile IDs", state.showTileIds, GRAY, LIGHTGRAY);
    if (showTileIdsButton.released) {
        state.showTileIds = !state.showTileIds;
    }
    UIState showEntityIdsButton = ui.ToggleButton("Entity IDs", state.showEntityIds, GRAY, LIGHTGRAY);
    if (showEntityIdsButton.released) {
        state.showEntityIds = !state.showEntityIds;
    }
    UIState showObjectsButton = ui.ToggleButton("Objects", state.showObjects, GRAY, LIGHTGRAY);
    if (showObjectsButton.released) {
        state.showObjects = !state.showObjects;
    }
    UIState showFloodDebug = ui.ToggleButton("Flood Debug", state.showFloodDebug, GRAY, LIGHTGRAY);
    if (showFloodDebug.released) {
        state.showFloodDebug = !state.showFloodDebug;
    }

    ui.Space({ 0, 8 });
    ui.Newline();

    for (int i = 0; i < EditMode_Count; i++) {
        if (ui.ToggleButton(EditModeStr((EditMode)i), mode == i, BLUE_DESAT, ColorBrightness(BLUE_DESAT, -0.3f)).pressed) {
            mode = (EditMode)i;
        }
        if (i % 6 == 5) {
            ui.Newline();
        }
    }

    ui.Space({ 0, 8 });
    ui.Newline();

    static ScrollPanel scrollPanel{};
    UIStyle scrollStyle = ui.GetStyle();
    scrollStyle.margin = {};
    scrollStyle.pad = {};
    scrollStyle.size = { width, g_RenderSize.y - ui.CursorScreen().y };
    ui.PushStyle(scrollStyle);
    ui.BeginScrollPanel(scrollPanel, IO::IO_ScrollPanelOuter);
    ui.PopStyle();

    switch (mode) {
        case EditMode_Maps: {
            DrawUI_MapActions(ui);
            break;
        }
        case EditMode_Tiles: {
            DrawUI_TileActions(ui);
            break;
        }
        case EditMode_Objects: {
            DrawUI_ObjectActions(ui);
            break;
        }
        case EditMode_Wang: {
            DrawUI_Wang(ui);
            break;
        }
        case EditMode_Paths: {
            DrawUI_PathActions(ui);
            break;
        }
        case EditMode_Dialog: {
            DrawUI_DialogActions(ui);
            break;
        }
        case EditMode_Entities: {
            DrawUI_EntityActions(ui);
            break;
        }
        case EditMode_PackFiles: {
            DrawUI_PackFiles(ui);
            break;
        }
        case EditMode_Debug: {
            DrawUI_Debug(ui);
            break;
        }
    }

    ui.EndScrollPanel(scrollPanel);
}
void Editor::DrawUI_MapActions(UI &ui)
{
    for (const Tilemap &map : pack_maps.tile_maps) {
        if (ui.Button(TextFormat("%s", map.name.c_str())).pressed) {
            map_id = map.id;
        }
        ui.Newline();
    }
}
void Editor::DrawUI_TileActions(UI &ui)
{
    const char *mapFileFilter[1] = { "*.png" };
    static const char *openRequest = 0;

#if 0
    const std::string &tilesetPath = rnStringCatalog.GetString(map.textureId);
    ui.Text(tilesetPath.c_str());
    if (ui.Button("Change tileset", ColorBrightness(ORANGE, -0.3f)).released) {
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
    ui.Newline();
#endif

    auto &map = pack_maps.FindById<Tilemap>(map_id);

    static float width = 0;
    static float height = 0;

    const uint32_t ctrlid_width = __COUNTER__;
    const uint32_t ctrlid_height = __COUNTER__;

    if (!UI::IsActiveEditor(ctrlid_width) && !UI::IsActiveEditor(ctrlid_height)) {
        width = map.width;
        height = map.height;
    }

    ui.Text("Size");
    ui.PushWidth(100);
    ui.Textbox(ctrlid_width, width);
    ui.Textbox(ctrlid_height, height);
    ui.PopStyle();
    if (ui.Button("Resize!").pressed) {
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
    ui.Newline();

    ui.Text("Flag");

    auto &tileEditMode = state.tiles.tileEditMode;
    if (ui.ToggleButton("Select", tileEditMode == TileEditMode_Select, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_Select;
    }
    if (ui.ToggleButton("Collision", tileEditMode == TileEditMode_Collision, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_Collision;
    }
    if (ui.ToggleButton("Auto-tile Mask", tileEditMode == TileEditMode_AutoTileMask, GRAY, SKYBLUE).pressed) {
        tileEditMode = TileEditMode_AutoTileMask;
    }
    ui.Newline();

    ui.Text("Layer");
    if (ui.ToggleButton("Ground", state.tiles.layer == TILE_LAYER_GROUND, GRAY, SKYBLUE).pressed) {
        state.tiles.layer = TILE_LAYER_GROUND;
    }
    if (ui.ToggleButton("Object", state.tiles.layer == TILE_LAYER_OBJECT, GRAY, SKYBLUE).pressed) {
        state.tiles.layer = TILE_LAYER_OBJECT;
    }
    ui.Newline();

    DrawUI_Tilesheet(ui);
}
void Editor::DrawUI_ObjectActions(UI &ui)
{
    auto &map = pack_maps.FindById<Tilemap>(map_id);
    Tilemap::Coord coord = state.objects.selected_coord;
    ObjectData *obj_data = map.GetObjectData(coord.x, coord.y);
    if (obj_data) {
        ui.Label("Object Data");
        ui.Newline();
        ui.HAQField(__COUNTER__, "", *obj_data, HAQ_EDIT, 100);
    } else {
        ui.Label("Create object:");
        ui.Newline();
        int create_type = 0;
        for (int i = 1; i < OBJ_COUNT; i++) {
            if (ui.Button(ObjTypeStr((ObjType)i)).pressed) {
                create_type = i;
            }
            ui.Newline();
        }
        if (create_type) {
            ObjectData obj{};
            obj.type = (ObjType)create_type;
            obj.x = coord.x;
            obj.y = coord.y;
            obj.tile = ObjTypeTileDefId(obj.type);
            map.object_data.push_back(obj);
        }
    }
}
void Editor::DrawUI_Tilesheet(UI &ui)
{
    // TODO(dlb): Support multi-select (big rectangle?), and figure out where this lives

    auto &tile_defs = pack_assets.tile_defs;
    TileLayerType layer = state.tiles.layer;
    auto &cursor = state.tiles.cursor;

    int tiles_x = 12;
    int tiles_y = tile_defs.size() / tiles_x + (tile_defs.size() % tiles_x > 0);

    UIStyle blackBorderStyle{};
    //blackBorderStyle.borderColor = BLACK;
    ui.PushStyle(blackBorderStyle);

    UIState sheet = ui.Checkerboard({ 0, 0, (float)TILE_W * tiles_x, (float)TILE_W * tiles_y });
    ui.Newline();

    // Draw tiledefs
    Vector2 imgTL{ sheet.contentRect.x, sheet.contentRect.y };
    for (size_t i = 0; i < tile_defs.size(); i++) {
        int tile_x = TILE_W * (i % tiles_x);
        int tile_y = TILE_W * (i / tiles_x);
        DrawTile(tile_defs[i].id, { imgTL.x + tile_x, imgTL.y + tile_y }, 0);
    }

    ui.PopStyle();

    // Draw collision overlay on tilesheet if we're in collision editing mode
    switch (state.tiles.tileEditMode) {
        case TileEditMode_Collision: {
            struct Flag {
                TileDef::Flags flag;
                char text;
                Color color;
            } flags[]{
                { TileDef::FLAG_SOLID , 'S', DARKGREEN },
                { TileDef::FLAG_LIQUID, 'L', BLUE      }
            };

            for (size_t i = 0; i < tile_defs.size(); i++) {
                const float tileThird = TILE_W / 3;
                const float tileSixth = tileThird / 2.0f;
                Vector2 cursor{
                    imgTL.x + TILE_W * (i % tiles_x),
                    imgTL.y + TILE_W * (i / tiles_x)
                };

                cursor.x++;
                cursor.y++;

                // TODO: Handle "y += .." case if we have more flags
                for (Flag flag : flags) {
                    Color color = flag.color;
                    char text = flag.text;
                    if (!(tile_defs[i].flags & flag.flag)) {
                        //text = '_';
                        color = Fade(GRAY, 0.7f);
                    }
                    Rectangle rect{ cursor.x, cursor.y, tileThird, tileThird };
                    DrawRectangleRec(rect, color);
                    DrawRectangleLinesEx(rect, 1, flag.color);
                    Vector2 size = dlb_MeasureTextShadowEx(fntSmall, &text, 1, 0);
                    Vector2 pos = cursor;
                    pos.x += tileSixth - size.x / 2.0f;
                    pos.y += tileSixth - size.y / 2.0f;
                    dlb_DrawTextShadowEx(fntSmall, &text, 1, pos, WHITE);
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

                const Color color = MAROON;

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

    if (cursor.selection_type == SELECTION_PALETTE) {
        if (cursor.selection_tiles[layer].size()) {
            // Draw highlight around currently selectedId tile(s)
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

    if (ui.Button("Add New Tile").pressed) {
        TileDef dat{};
        pack_assets.Add<TileDef>(dat);
    }
    ui.Newline();

    // Draw editor info at bottom
    if (cursor.selection_tiles[layer].size() == 1) {
        uint16_t tile_def_id = cursor.selection_tiles[layer][0];
        if (tile_def_id) {
            TileDef &tile_def = pack_assets.FindById<TileDef>(tile_def_id);
            ui.HAQField(__COUNTER__, "", tile_def, HAQ_EDIT, 110.0f);
        }
    }
}
void Editor::DrawUI_Wang(UI &ui)
{
    if (ui.Button("Generate template").pressed) {
        Err err = WangTileset::GenerateTemplate("resources/wang/template.png");
        if (err) {
            // TODO: Show the user
            printf("[editor] Failed to generate wang tileset template\n");
        }
    }
    ui.Newline();

    WangTileset &wangTileset = state.wang.wangTileset;
    WangMap &wangMap = state.wang.wangMap;

    UIStyle uiWangStyle4x{};
    uiWangStyle4x.scale = 2;
    uiWangStyle4x.borderColor[UI_CtrlTypeDefault] = BLACK;
    uiWangStyle4x.borderWidth[UI_CtrlTypeDefault] = 1;
    ui.PushStyle(uiWangStyle4x);

    UIStyle styleSelected{ uiWangStyle4x };
    styleSelected.borderColor[UI_CtrlTypeDefault] = WHITE;
    styleSelected.borderWidth[UI_CtrlTypeDefault] = 1;

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
            ui.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (ui.Image(wangTileset.thumbnail, srcRect).pressed) {
            state.wang.hTex = state.wang.hTex == i ? -1 : i;
            state.wang.vTex = -1;
        }
        if (stylePushed) {
            ui.PopStyle();
        }
    }
    Vector2 cursorEndOfFirstLine = ui.CursorScreen();
    ui.Newline();

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
            ui.PushStyle(styleSelected);
            stylePushed = true;
        }
        if (ui.Image(wangTileset.thumbnail, srcRect).pressed) {
            state.wang.hTex = -1;
            state.wang.vTex = state.wang.vTex == i ? -1 : i;
        }
        if (stylePushed) {
            ui.PopStyle();
        }
    }
    ui.Newline();

    if (ui.Button("Export tileset").pressed) {
        ExportImage(wangTileset.ts.img, wangTileset.filename.c_str());
    }
    ui.Newline();
    ui.Image(wangTileset.thumbnail);
    ui.Newline();

    ui.PopStyle();

    auto &map = pack_maps.FindById<Tilemap>(map_id);

    if (ui.Button("Re-generate Map").pressed) {
        wangTileset.GenerateMap(map.width, map.height, wangMap);
    }
    ui.Newline();

    if (ui.Image(wangMap.colorized).pressed) {
        map.SetFromWangMap(wangMap, now);
    }

    DrawUI_WangTile();
}
void Editor::DrawUI_WangTile(void)
{
    WangTileset &wangTileset = state.wang.wangTileset;
    auto &tile_defs = pack_assets.tile_defs;

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

        Vector2 uiPosition{ wangBg.x + 8, wangBg.y + 8 };
        Vector2 uiSize{ 200, 200 };
        UI uiWangTile{ uiPosition, uiSize, uiWangTileStyle };

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

                    const GfxFrame &gfx_frame = GetTileGfxFrame(selectedTile);
                    const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);
                    const Rectangle tileRect = TileDefRect(tile);
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

                    const GfxFrame &gfx_frame = GetTileGfxFrame(selectedTile);
                    const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);
                    const Rectangle tileRect = TileDefRect(tile);
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

            Vector2 uiPosition{ wangBg.x + wangBg.width + 8 , wangBg.y };
            Vector2 uiSize{ TILE_W * 8, TILE_W * 8 };
            UI uiWangNew{ uiPosition, uiSize, {} };
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
void Editor::DrawUI_PathActions(UI &ui)
{
    if (state.pathNodes.cursor.dragging) {
        printf("dragging\n");
    }
    ui.Text(TextFormat(
        "drag: %s, path: %d, node: %d",
        state.pathNodes.cursor.dragging ? "true" : "false",
        state.pathNodes.cursor.dragPathId,
        state.pathNodes.cursor.dragPathNodeIndex
    ));
}
void Editor::DrawUI_DialogActions(UI &ui)
{
#if 0
    //if (ui.ToggleButton("Add", DARKGREEN).pressed) {
    //    //map.warps.push_back({});
    //}
    //ui.Newline();

    UIStyle searchStyle = ui.GetStyle();
    searchStyle.size.x = 400;
    searchStyle.pad = UIPad(8, 2);
    searchStyle.margin = UIMargin(0, 0, 4, 6);
    ui.PushStyle(searchStyle);

    static STB_TexteditState txtSearch{};
    static std::string filter{ "*" };
    ui.Newline();
    ui.PushWidth(0);
    if (ui.Button("Clear").pressed) filter = "";
    if (ui.Button("All").pressed) filter = "*";
    ui.PopStyle();
    ui.Textbox(txtSearch, filter);

    ui.Newline();

    for (Dialog &dialog : dialog_library.dialogs) {
        if (!dialog.id) {
            continue;
        }

        Color bgColor = dialog.id == state.dialog.dialogId ? SKYBLUE : BLUE;
        std::string msg{ dialog.msg };
        if (!StrFilter(msg.c_str(), filter.c_str())) {
            continue;
        }

        if (ui.Text(msg.c_str(), WHITE, bgColor).down) {
            state.dialog.dialogId = dialog.id;
        }
        ui.Newline();
    }

    if (state.dialog.dialogId) {
        Dialog *dialog = dialog_library.FindById(state.dialog.dialogId);
        if (dialog) {
            ui.Text("message");
            ui.Newline();
            static STB_TexteditState txtMessage{};
            ui.Textbox(txtMessage, dialog.msg);
            ui.Newline();

            static STB_TexteditState txtOptionId[SV_MAX_ENTITY_DIALOG_OPTIONS]{};
            for (int i = 0; i < SV_MAX_ENTITY_DIALOG_OPTIONS; i++) {
                ui.Text(TextFormat("option %d", i));
                ui.Newline();
                float id = dialog.option_ids[i];
                ui.Textbox(txtOptionId[i], id, 0, "%.f");
                dialog.option_ids[i] = (DialogId)CLAMP(id, 0, pack_assets.dialogs.size() - 1);
                ui.Newline();
            }
        } else {
            state.dialog.dialogId = 0;
        }
    }

    ui.PopStyle();
#endif
}
void Editor::DrawUI_EntityActions(UI &ui)
{
    auto &map = pack_maps.FindById<Tilemap>(map_id);
    if (ui.Button("Despawn all", ColorBrightness(MAROON, -0.3f)).pressed) {
        for (const Entity &entity : entityDb->entities) {
            if (entity.type == Entity::TYP_PLAYER || entity.map_id != map.id) {
                continue;
            }
            assert(entity.id && entity.type);
            entityDb->DespawnEntity(entity.id, now);
        }
    }

    ui.Newline();
    ui.Space({ 0, 4 });

    static SearchBox searchEntities{ "Search entities..." };
    ui.PushWidth(400);
    BeginSearchBox(ui, searchEntities);
    ui.PopStyle();
    ui.Newline();

    for (uint32_t i = 0; i < SV_MAX_ENTITIES; i++) {
        Entity &entity = entityDb->entities[i];
        if (!entity.id || entity.map_id != map.id) {
            continue;
        }

        const char *idStr = TextFormat("[%d] %s", entity.id, EntityTypeStr(entity.type));
        if (!StrFilter(idStr, searchEntities.filter.c_str())) {
            continue;
        }

        Color bgColor = entity.id == state.entities.selectedId ? ColorBrightness(ORANGE, -0.3f) : BLUE_DESAT;
        if (ui.Text(idStr, WHITE, bgColor).down) {
            state.entities.selectedId = entity.id;
        }
        ui.Newline();
    }
    ui.Newline();

    if (state.entities.selectedId) {
        Entity *entity = entityDb->FindEntity(state.entities.selectedId);
        if (entity) {
            const int labelWidth = 100;

            ////////////////////////////////////////////////////////////////////////
            // Entity
            ui.Label("id", labelWidth);
            ui.Text(TextFormat("%d", entity->id));
            ui.Newline();

            ui.Label("type", labelWidth);
            ui.Text(EntityTypeStr(entity->type));
            ui.Newline();

            ui.Label("position", labelWidth);
            ui.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeTextbox);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->position.x);
            ui.PopStyle();
            ui.PopStyle();

            ui.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeTextbox);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->position.y);
            ui.PopStyle();
            ui.PopStyle();
            ui.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Combat
            ui.Label("attk cooldown", labelWidth);
            const float attackCooldownLeft = MAX(0, entity->attack_cooldown - (now - entity->last_attacked_at));
            ui.Text(TextFormat("%.3f", attackCooldownLeft));
            ui.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Collision
            ui.Label("radius", labelWidth);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->radius);
            ui.PopStyle();
            ui.Newline();

            ui.Label("colliding", labelWidth);
            if (entity->colliding) {
                ui.Text("True", RED);
            } else {
                ui.Text("False", WHITE);
            }
            ui.Newline();

            ui.Label("onWarp", labelWidth);
            if (entity->on_warp) {
                ui.Text("True", SKYBLUE);
            } else {
                ui.Text("False", WHITE);
            }
            ui.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Life
            ui.Label("health", labelWidth);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->hp);
            ui.PopStyle();

            ui.Text("/");

            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->hp_max);
            ui.PopStyle();
            ui.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Physics
            ui.Label("drag", labelWidth);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->drag);
            ui.PopStyle();
            ui.Newline();

            ui.Label("speed", labelWidth);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->speed);
            ui.PopStyle();
            ui.Newline();

            ui.Label("velocity", labelWidth);
            ui.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeTextbox);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->velocity.x);
            ui.PopStyle();
            ui.PopStyle();
            ui.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeTextbox);
            ui.PushWidth(80);
            ui.Textbox(__COUNTER__, entity->velocity.y);
            ui.PopStyle();
            ui.PopStyle();
            ui.Newline();
        } else {
            state.entities.selectedId = 0;
        }
    }
}
void Editor::DrawUI_PackFiles(UI &ui)
{
    static SearchBox searchBox{ "Search packs..." };
    ui.PushWidth(width - 74);
    BeginSearchBox(ui, searchBox);
    ui.PopStyle();

    if (ui.Button("Clear", searchBox.filter.size() ? ColorBrightness(MAROON, -0.3f) : GRAY).pressed) {
        searchBox.filter = "";
    }
    ui.Newline();

    ui.PushSize({ width - 12, 60 });
    ui.PushPadding({});
    ui.BeginScrollPanel(searchBox.panel, IO::IO_ScrollPanelInner);
    ui.PopStyle();
    ui.PopStyle();

    ui.PushSize({ width - 30, 0 });
    ui.PushMargin({});
    ui.PushPadding({ 0, 0, 0, 4 });

    Pack *packs[]{ &pack_assets, &pack_maps };
    for (Pack *packPtr : packs) {
        Pack &pack = *packPtr;
        if (!pack.version) {
            continue;
        }

        Color bgColor = &pack == state.packFiles.selectedPack ? SKYBLUE : BLUE_DESAT;
        const char *idStr = pack.name.c_str();
        if (!StrFilter(idStr, searchBox.filter.c_str())) {
            continue;
        }

        if (ui.Text(idStr, WHITE, bgColor).down) {
            state.packFiles.selectedPack = &pack;
        }
        ui.Newline();
    }

    ui.PopStyle();
    ui.PopStyle();
    ui.PopStyle();

    ui.EndScrollPanel(searchBox.panel);
    ui.Newline();

    if (state.packFiles.selectedPack) {
        Pack &pack = *state.packFiles.selectedPack;

        ui.PushWidth(100);

        ui.Label("version");
        ui.Text(TextFormat("%d", pack.version));
        ui.Newline();

        ui.Label("tocEntries");
        ui.Text(TextFormat("%zu", pack.toc.entries.size()));
        ui.Newline();

        ui.PopStyle();

        static struct DatTypeFilter {
            bool enabled;
            const char *text;
            Color color;
        } datTypeFilter[DAT_TYP_COUNT]{
            { true, "GFX", ColorFromHSV((int)DAT_TYP_GFX_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "MUS", ColorFromHSV((int)DAT_TYP_MUS_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "SFX", ColorFromHSV((int)DAT_TYP_SFX_FILE  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "FRM", ColorFromHSV((int)DAT_TYP_GFX_FRAME * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "ANM", ColorFromHSV((int)DAT_TYP_GFX_ANIM  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "SPT", ColorFromHSV((int)DAT_TYP_SPRITE    * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "DEF", ColorFromHSV((int)DAT_TYP_TILE_DEF  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "MAT", ColorFromHSV((int)DAT_TYP_TILE_MAT  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "MAP", ColorFromHSV((int)DAT_TYP_TILE_MAP  * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true, "ENT", ColorFromHSV((int)DAT_TYP_ENTITY    * (360.0f / (float)DAT_TYP_COUNT), 0.9f, 0.6f) },
        };

        const bool datTypeEnabled[DAT_TYP_COUNT]{
            (bool)pack.gfx_files .size(),
            (bool)pack.mus_files .size(),
            (bool)pack.sfx_files .size(),
            (bool)pack.gfx_frames.size(),
            (bool)pack.gfx_anims .size(),
            (bool)pack.sprites   .size(),
            (bool)pack.tile_defs .size(),
            (bool)pack.tile_mats .size(),
            (bool)pack.tile_maps .size(),
            (bool)pack.entities  .size(),
        };

        ui.PushWidth(34);
        for (int i = 0; i < DAT_TYP_COUNT; i++) {
            DatTypeFilter &filter = datTypeFilter[i];
            if (ui.ToggleButton(datTypeEnabled[i] ? filter.text : "---", filter.enabled, DARKGRAY, filter.color).pressed) {
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
                ui.Newline();
            }
        }
        ui.PopStyle();
        ui.Space({ 0, 4 });

        ui.PushWidth(width - 20);

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

            ui.PushMargin({ 1, 0, 0, 4 });

            const char *text = TextFormat("[%s] %s", selected ? "-" : "+", desc);
            if (ui.Text(text, WHITE, ColorBrightness(filter.color, -0.2f)).pressed) {
                if (!selected) {
                    newSelectedOffset = entry.offset;
                } else {
                    state.packFiles.selectedPackEntryOffset = 0;
                }
            }

            ui.PopStyle();
            ui.Newline();

            if (selected) {
                #define HAQ_UI_FIELD(c_type, c_name, c_init, flags, condition, parent) \
                    if (condition) { \
                        ui.HAQField(__COUNTER__, #c_name, parent.c_name, (flags), labelWidth); \
                    }

                #define HAQ_UI(hqt, parent) \
                    hqt(HAQ_UI_FIELD, parent)

                const int labelWidth = 100;
                switch (entry.dtype) {
                    case DAT_TYP_GFX_FILE:
                    {
                        GfxFile &gfx_file = pack.gfx_files[entry.index];
                        ui.HAQField(__COUNTER__, "", gfx_file, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_MUS_FILE:
                    {
                        MusFile &mus_file = pack.mus_files[entry.index];
                        ui.HAQField(__COUNTER__, "", mus_file, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_SFX_FILE:
                    {
                        SfxFile &sfx_file = pack.sfx_files[entry.index];
                        ui.HAQField(__COUNTER__, "", sfx_file, HAQ_EDIT, labelWidth);

                        if (!IsSoundPlaying(sfx_file.name)) {
                            if (ui.Button("Play", ColorBrightness(DARKGREEN, -0.3f)).pressed) {
                                PlaySound(sfx_file.name, 0);
                            }
                        } else {
                            if (ui.Button("Stop", ColorBrightness(MAROON, -0.3f)).pressed) {
                                StopSound(sfx_file.name);
                            }
                        }
                        ui.Newline();
                        break;
                    }
                    case DAT_TYP_GFX_FRAME:
                    {
                        GfxFrame &gfx_frame = pack.gfx_frames[entry.index];
                        ui.HAQField(__COUNTER__, "", gfx_frame, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_GFX_ANIM:
                    {
                        GfxAnim &gfx_anim = pack.gfx_anims[entry.index];
                        ui.HAQField(__COUNTER__, "", gfx_anim, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_SPRITE:
                    {
                        const float labelWidth = 40.0f;
                        Sprite &sprite = pack.sprites[entry.index];
                        ui.HAQField(__COUNTER__, "", sprite, HAQ_EDIT, labelWidth);
#if 0
                        ui.Label("name", labelWidth);
                        ui.Text(CSTRS(sprite.name));
                        ui.Newline();

                        ui.Label("N ", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_N]));
                        ui.Newline();

                        ui.Label("E ", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_E]));
                        ui.Newline();

                        ui.Label("S ", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_S]));
                        ui.Newline();

                        ui.Label("S ", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_W]));
                        ui.Newline();

                        ui.Label("NE", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_SE]));
                        ui.Newline();

                        ui.Label("SE", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_SE]));
                        ui.Newline();

                        ui.Label("SW", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_SW]));
                        ui.Newline();

                        ui.Label("NW", labelWidth);
                        ui.Text(CSTRS(sprite.anims[DIR_NW]));
                        ui.Newline();
#endif
                        break;
                    }
                    case DAT_TYP_TILE_DEF:
                    {
                        TileDef &tile_def = pack.tile_defs[entry.index];
                        ui.HAQField(__COUNTER__, "", tile_def, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_TILE_MAT:
                    {
                        TileMat &tile_mat = pack.tile_mats[entry.index];
                        ui.HAQField(__COUNTER__, "", tile_mat, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_TILE_MAP:
                    {
                        Tilemap &tile_map = pack.tile_maps[entry.index];
                        ui.HAQField(__COUNTER__, "", tile_map, HAQ_EDIT, labelWidth);
                        break;
                    }
                    case DAT_TYP_ENTITY:
                    {
                        ui.Text("TODO");
                        ui.Newline();
                        break;
                    }
                }

                #undef HAQ_UI_TYPE
                #undef HAQ_UI_FIELD
                #undef HAQ_UI
            }
        }

        ui.PopStyle();

        if (newSelectedOffset) {
            state.packFiles.selectedPackEntryOffset = newSelectedOffset;
        }
    }
}
void Editor::DrawUI_Debug(UI &ui)
{
#if 0
    static UID uid = NextUID();
    ui.Text(TextFormat("%.*s", 20, uid.bytes));
    if (true) {// || ui.ToggleButton("NextUID").pressed) {
        uid = NextUID();
    }
    ui.Newline();
#endif

    const Color colorOff = BLUE_DESAT;
    const Color colorOn = ColorBrightness(BLUE_DESAT, -0.3f);

    ui.PushWidth(80);

    if (ui.ToggleButton("Frames", showGfxFrameEditor, colorOff, colorOn).pressed) {
        bool show = !showGfxFrameEditor;
        ResetEditorFlags();
        showGfxFrameEditor = show;
        showGfxFrameEditorDirty = true;
    }

    if (ui.ToggleButton("Animations", showGfxAnimEditor, colorOff, colorOn).pressed) {
        bool show = !showGfxAnimEditor;
        ResetEditorFlags();
        showGfxAnimEditor = show;
        showGfxAnimEditorDirty = true;
    }

    if (ui.ToggleButton("Sprites", showSpriteEditor, colorOff, colorOn).pressed) {
        bool show = !showSpriteEditor;
        ResetEditorFlags();
        showSpriteEditor = show;
        showSpriteEditorDirty = true;
    }

    if (ui.ToggleButton("Tile Defs", showTileDefEditor, colorOff, colorOn).pressed) {
        bool show = !showTileDefEditor;
        ResetEditorFlags();
        showTileDefEditor = show;
        showTileDefEditorDirty = true;
    }
    ui.Newline();

    ui.PopStyle();

    ///////////////////////////////////////////////////////////////////////////////////////////

    if (showGfxFrameEditor) {
        static SearchBox searchGfxFiles{ "Search graphics..." };
        PackSearchBox<GfxFile>(ui, pack_assets, searchGfxFiles);

        GfxFile &gfx_file = pack_assets.FindById<GfxFile>(state.selections.byType[GfxFile::dtype]);
        if (gfx_file.id == state.selections.byType[GfxFile::dtype]) {
            ui.HAQField(__COUNTER__, "", gfx_file, HAQ_EDIT, 80.0f);
        }

        ui.Text("--------------------------------------------------");
        ui.Newline();

        // TODO: Toggle only showing frames for selected gfx file
        // TODO: Enter x/y and have frames auto-generated for gfx file

        static float frames_x{ 1 };
        static float frames_y{ 1 };

        ui.Label("X");
        ui.PushWidth(24);
        ui.Textbox(__COUNTER__, frames_x);
        ui.PopStyle();

        ui.Label("Y");
        ui.PushWidth(24);
        ui.Textbox(__COUNTER__, frames_y);
        ui.PopStyle();

        frames_x = MAX(1, frames_x);
        frames_y = MAX(1, frames_y);
        if (ui.Button("Split frames!").pressed) {
            int x_max = roundf(frames_x);
            int y_max = roundf(frames_y);
            int frame_w = gfx_file.texture.width / x_max;
            int frame_h = gfx_file.texture.height / y_max;
            for (int y = 0; y < y_max; y++) {
                for (int x = 0; x < x_max; x++) {
                    GfxFrame gfx_frame{};
                    gfx_frame.gfx = gfx_file.name;
                    gfx_frame.x = x * frame_w;
                    gfx_frame.y = y * frame_h;
                    gfx_frame.w = frame_w;
                    gfx_frame.h = frame_h;
                    pack_assets.Add<GfxFrame>(gfx_frame);
                }
            }
        }
        ui.Newline();

        static bool filterByGfxFile = true;
        const char *filterByGfxText = TextFormat("[%s] Filter by GfxFile", filterByGfxFile ? "X" : " ");
        if (ui.ToggleButton(filterByGfxText, filterByGfxFile, BLUE_DESAT, BLUE).pressed) {
            filterByGfxFile = !filterByGfxFile;
        }
        ui.Newline();

        ui.Text("--------------------------------------------------");
        ui.Newline();

        // TODO: This should have a custom filter that only shows frames in the selected GfxFile
        static SearchBox searchGfxFrames{ "Search frames..." };
        PackSearchBox<GfxFrame>(ui, pack_assets, searchGfxFrames, filterByGfxFile ? FilterGfxFrameByGfxFile : 0);

        GfxFrame &gfx_frame = pack_assets.FindById<GfxFrame>(state.selections.byType[GfxFrame::dtype]);
        if (gfx_frame.id == state.selections.byType[GfxFrame::dtype]) {
            ui.HAQField(__COUNTER__, "", gfx_frame, HAQ_EDIT, 80.0f);
        }
    }

    if (showGfxAnimEditor) {
        static SearchBox searchGfxAnims{ "Search animations..." };
        PackSearchBox<GfxAnim>(ui, pack_assets, searchGfxAnims);
        GfxAnim &gfx_anim = pack_assets.FindById<GfxAnim>(state.selections.byType[GfxAnim::dtype]);
        if (gfx_anim.id == state.selections.byType[GfxAnim::dtype]) {
            ui.HAQField(__COUNTER__, "", gfx_anim, HAQ_EDIT, 80.0f);
        }

        ui.Text("--------------------------------------------------");
        ui.Newline();

#if 0
        bool addFrame = false;
        if (state.selections.byType[GfxFrame::dtype]) {
            addFrame = ui.Button("Add frame").pressed;
        } else {
            ui.Label("Select a frame to add it to the animation");
            ui.Space({ 0, 2 });
        }
        ui.Newline();

        ui.Text("--------------------------------------------------");
        ui.Newline();
#endif

        static SearchBox searchGfxFrame{ "Search frames..." };
        PackSearchBox<GfxFrame>(ui, pack_assets, searchGfxFrame);
        GfxFrame &gfx_frame = pack_assets.FindById<GfxFrame>(state.selections.byType[GfxFrame::dtype]);
        ui.HAQField(__COUNTER__, "", gfx_frame, HAQ_EDIT, 80.0f);

#if 0
        if (addFrame && gfx_anim.id && gfx_frame.id) {
            gfx_anim.frames.push_back(gfx_frame.name);
        }
#endif
    }
}

void Editor::DrawUI_GfxFrameEditor(void)
{
    const float borderWidth = 2;  // hack to prevent double borders
    Vector2 uiPosition{};
    if (dock_left) {
        uiPosition = { width - borderWidth, 0 };
    } else {
        uiPosition = { 0, 0 };
    }
    Vector2 uiSize{ (float)g_RenderSize.x - (width - borderWidth), g_RenderSize.y };

    UI ui{ uiPosition, uiSize, {} };

    static ScrollPanel scrollPanel{ true };
    ui.PushMargin({});
    ui.PushSize(uiSize);
    ui.BeginScrollPanel(scrollPanel, IO::IO_ScrollPanelOuter);
    ui.PopStyle();
    ui.PopStyle();

    const GfxFile &gfx_file = pack_assets.FindById<GfxFile>(state.selections.byType[GfxFile::dtype]);
    if (gfx_file.id == state.selections.byType[GfxFile::dtype]) {
        const Vector2 cursor = ui.CursorScreen();
        Vector2 imageTL = cursor;

        const UIStyle style = ui.GetStyle();
        imageTL.x += style.margin.left + style.borderWidth[UI_CtrlTypeDefault];
        imageTL.y += style.margin.top  + style.borderWidth[UI_CtrlTypeDefault];

        UIState imgState = ui.Image(gfx_file.texture, {}, true);
        ui.Newline();

        const auto &frames = pack_assets.gfx_frame_ids_by_gfx_file_name.find(gfx_file.name);
        if (frames != pack_assets.gfx_frame_ids_by_gfx_file_name.end()) {
            for (uint16_t &gfx_frame_id : frames->second) {
                const GfxFrame &gfx_frame = pack_assets.FindById<GfxFrame>(gfx_frame_id);
                Rectangle frame_rect{
                    imageTL.x + gfx_frame.x,
                    imageTL.y + gfx_frame.y,
                    (float)gfx_frame.w,
                    (float)gfx_frame.h
                };

                const bool selected = gfx_frame.id == state.selections.byType[GfxFrame::dtype];
                if (selected) {
                    DrawRectangleLinesEx(frame_rect, 2, MAGENTA);
                } else {
                    DrawRectangleLinesEx(frame_rect, 1, Fade(LIGHTGRAY, 0.7f));
                }

                const bool hover = !io.MouseCaptured() && dlb_CheckCollisionPointRec(GetMousePosition(), frame_rect);
                if (hover) {
                    frame_rect = RectShrink(frame_rect, 2);
                    DrawRectangleLinesEx(frame_rect, 2, YELLOW);

                    ui.Tooltip(gfx_frame.name, { frame_rect.x + 10, frame_rect.y + 6 });

                    // Select frame
                    if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        state.selections.byType[GfxFrame::dtype] = gfx_frame.id;
                    }
                }
            }
        }
    }

    ui.EndScrollPanel(scrollPanel);
}
void Editor::DrawUI_GfxAnimEditor(void)
{
    const float borderWidth = 2;  // hack to prevent double borders
    Vector2 uiPosition{};
    if (dock_left) {
        uiPosition = { width - borderWidth, 0 };
    } else {
        uiPosition = { 0, 0 };
    }
    Vector2 uiSize{ g_RenderSize.x - (width - borderWidth), g_RenderSize.y };

    UI ui{ uiPosition, uiSize, {} };

    static ScrollPanel scrollPanel{ true };
    ui.PushMargin({});
    ui.PushSize(uiSize);
    ui.BeginScrollPanel(scrollPanel, IO::IO_ScrollPanelOuter);
    ui.PopStyle();
    ui.PopStyle();

    GfxAnim &gfx_anim = pack_assets.FindById<GfxAnim>(state.selections.byType[GfxAnim::dtype]);
    if (gfx_anim.id == state.selections.byType[GfxAnim::dtype]) {
        ui.Text("Preview");
        ui.Newline();

        static uint16_t anim_id{};
        static GfxAnimState anim_state{};
        static bool anim_paused{};
        static uint16_t frame_selected{};

        if (gfx_anim.id != anim_id) {
            anim_state = {};
            anim_id = gfx_anim.id;
        }

        bool add = io.KeyPressed(KEY_INSERT);
        if (add) {
            if (state.selections.byType[GfxFrame::dtype]) {
                GfxFrame &gfx_frame = pack_assets.FindById<GfxFrame>(state.selections.byType[GfxFrame::dtype]);
                if (gfx_frame.id) {
                    gfx_anim.frames.push_back(gfx_frame.name);
                }
            }
        }

        if (gfx_anim.frames.size()) {
            // In case # of frames changed via editor, we clamp
            anim_state.frame = CLAMP(anim_state.frame, 0, gfx_anim.frames.size() - 1);
            frame_selected = CLAMP(frame_selected, 0, gfx_anim.frames.size() - 1);
            if (!anim_paused) {
                UpdateGfxAnim(gfx_anim, dt, anim_state);
            }

            GfxFrame &gfx_frame = pack_assets.FindByName<GfxFrame>(gfx_anim.frames[anim_state.frame]);
            Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
            GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);

            ui.Image(gfx_file.texture, rect, true);
            ui.Newline();

            ui.Label(TextFormat("Frame %u of %u", anim_state.frame + 1, gfx_anim.frames.size()));
            ui.Newline();

            bool home = io.KeyPressed(KEY_HOME);
            bool prev = io.KeyPressed(KEY_LEFT, true);
            bool play = io.KeyPressed(KEY_SPACE);
            bool next = io.KeyPressed(KEY_RIGHT, true);
            bool end  = io.KeyPressed(KEY_END);
            bool del  = io.KeyPressed(KEY_DELETE);

            if (add) printf("add\n");
            if (del) printf("del\n");

            home |= ui.Button("|<", home ? SKYBLUE : GRAY).pressed;
            prev |= ui.Button("< ", prev ? SKYBLUE : GRAY).pressed;
            play |= ui.Button(anim_paused ? "|>" : "||", anim_paused ? LIME : MAROON).pressed;
            next |= ui.Button(" >", next ? SKYBLUE : GRAY).pressed;
            end  |= ui.Button(">|", end  ? SKYBLUE : GRAY).pressed;
            ui.Newline();

            if (home) {
                anim_paused = true;
                anim_state.frame = 0;
            }
            if (prev) {
                anim_paused = true;
                if (anim_state.frame) {
                    anim_state.frame--;
                } else {
                    anim_state.frame = gfx_anim.frames.size() - 1;
                }
            }
            if (play) {
                anim_paused = !anim_paused;
            }
            if (next) {
                anim_paused = true;
                if (anim_state.frame < gfx_anim.frames.size() - 1) {
                    anim_state.frame++;
                } else {
                    anim_state.frame = 0;
                }
            }
            if (end) {
                anim_paused = true;
                anim_state.frame = gfx_anim.frames.size() - 1;
            }

            ui.Text("Frames (Ins = add, Del = remove, Drag = swap)");
            ui.Newline();

            if (del) {
                gfx_anim.frames.erase(gfx_anim.frames.begin() + frame_selected);
            }

            uint16_t frame_idx = 0;
            for (const std::string &frame : gfx_anim.frames) {
                GfxFrame &gfx_frame = pack_assets.FindByName<GfxFrame>(frame);
                Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
                GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);

                const Rectangle &uiRect = ui.Rect();
                if (ui.CursorScreen().x + rect.width > uiRect.x + uiRect.width) {
                    ui.Newline();
                }

                UIState state = ui.Image(gfx_file.texture, rect, true);

                if (state.pressed) {
                    frame_selected = frame_idx;
                } else if (state.released) {
                    std::swap(gfx_anim.frames[frame_selected], gfx_anim.frames[frame_idx]);
                }

                if (frame_idx == frame_selected) {
                    DrawRectangleLinesEx(state.contentRect, 2, MAGENTA);
                } else if (state.hover) {
                    DrawRectangleLinesEx(state.contentRect, 2, YELLOW);
                }

                if (state.hover) {
                    ui.Tooltip(gfx_frame.name, { state.contentRect.x + 10, state.contentRect.y + 6 });
                }

                frame_idx++;
            }
            ui.Newline();
        } else {
            ui.Label("No frames");
        }
    }

    ui.EndScrollPanel(scrollPanel);
}
void Editor::DrawUI_SpriteEditor(void)
{
}
void Editor::DrawUI_TileDefEditor(void)
{
}