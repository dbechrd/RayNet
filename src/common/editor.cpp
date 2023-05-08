#include "editor.h"
#include "ui/ui.h"
#include "tinyfiledialogs.h"

const char *EditModeStr(EditMode mode)
{
    switch (mode) {
        case EditMode_Tiles: return "Tiles";
        case EditMode_Paths: return "Paths";
        default: return "<null>";
    }
}

void Editor::HandleInput(IO &io, Camera2D &camera)
{
    if (io.KeyPressed(KEY_GRAVE)) {
        active = !active;
    }

    if (active) {
        if (io.KeyDown(KEY_LEFT_CONTROL) && io.KeyPressed(KEY_ZERO)) {
            camera.zoom = 1.0f;
        }
    }
}

void Editor::DrawOverlays(IO &io, Tilemap &map, Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorOverlay);

    HandleInput(io, camera);

    // Draw tile collision layer
    if (state.showColliders) {
        map.DrawColliders(camera);
    }

    if (active) {
        switch (mode) {
            case EditMode_Tiles: {
                DrawOverlay_Tiles(io, map, camera, now);
                break;
            }
            case EditMode_Paths: {
                DrawOverlay_Paths(io, map, camera);
                break;
            }
        }
    }

    io.PopScope();
}

void Editor::DrawOverlay_Tiles(IO &io, Tilemap &map, Camera2D &camera, double now)
{
    if (!io.MouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPlaceTile = io.MouseButtonDown(MOUSE_BUTTON_LEFT);
        const bool editorPickTile = io.MouseButtonDown(MOUSE_BUTTON_MIDDLE);
        const bool editorFillTile = io.KeyPressed(KEY_F);

        Tile hoveredTile;
        bool hoveringTile = map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile);
        if (hoveringTile) {
            Tilemap::Coord coord{};
            bool validCoord = map.WorldToTileIndex(cursorWorldPos.x, cursorWorldPos.y, coord);
            assert(validCoord);  // should always be true when hoveredTile != null

            if (editorPlaceTile) {
                map.Set(coord.x, coord.y, state.tiles.cursor.tileDefId, now);
            } else if (editorPickTile) {
                state.tiles.cursor.tileDefId = hoveredTile;
            } else if (editorFillTile) {
                map.Fill(coord.x, coord.y, state.tiles.cursor.tileDefId, now);
            }

            DrawRectangleLinesEx({ (float)coord.x * TILE_W, (float)coord.y * TILE_W, TILE_W, TILE_W }, 2, WHITE);
        }
    }
}

void Editor::DrawOverlay_Paths(IO &io, Tilemap &map, Camera2D &camera)
{
    auto &cursor = state.pathNodes.cursor;
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

    // Draw path edges
    for (uint32_t aiPathId = 0; aiPathId < map.pathCount; aiPathId++) {
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
    for (uint32_t aiPathId = 0; aiPathId < map.pathCount; aiPathId++) {
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

UIState Editor::DrawUI(IO &io, Vector2 position, Tilemap &map, double now)
{
    if (!active) return {};
    io.PushScope(IO::IO_EditorUI);

    UIState state = DrawUI_ActionBar(io, position, map, now);
    if (state.hover) {
        io.CaptureMouse();
    }

    io.PopScope();
    return state;
}

UIState Editor::DrawUI_ActionBar(IO &io, Vector2 position, Tilemap &map, double now)
{
    UIStyle uiActionBarStyle{};
    UI uiActionBar{ position, uiActionBarStyle };

    // TODO: UI::Panel
    UIState uiState{};
    const Rectangle actionBarRect{ position.x, position.y, 800, 110 };
    DrawRectangleRounded(actionBarRect, 0.15f, 8, ASESPRITE_BEIGE);
    DrawRectangleRoundedLines(actionBarRect, 0.15f, 8, 2.0f, BLACK);
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);

    const char *mapFileFilter[1] = { "*.dat" };
    static const char *openRequest = 0;
    static const char *saveAsRequest = 0;

    UIState openButton = uiActionBar.Button("Open");
    if (openButton.clicked) {
        const char *filename = map.filename;
        std::thread openFileThread([filename, mapFileFilter]{
            openRequest = tinyfd_openFileDialog(
                "Open File",
                filename,
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.dat)",
                0
            );
        });
        openFileThread.detach();
    }
    if (openRequest) {
        Err err = map.Load(openRequest, now);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to load file %s. %s\n", openRequest, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        openRequest = 0;
    }

    UIState saveButton = uiActionBar.Button("Save");
    if (saveButton.clicked) {
        Err err = map.Save(map.filename);
        if (err) {
            const char *filename = map.filename;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", filename, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState saveAsButton = uiActionBar.Button("Save As");
    if (saveAsButton.clicked) {
        const char *filename = map.filename;
        std::thread saveAsThread([filename, mapFileFilter]{
            saveAsRequest = tinyfd_saveFileDialog(
                "Save File",
                filename,
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.dat)");
        });
        saveAsThread.detach();
    }
    if (saveAsRequest) {
        Err err = map.Save(saveAsRequest);
        if (err) {
            std::thread errorThread([err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", saveAsRequest, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
        saveAsRequest = 0;
    }

    UIState reloadButton = uiActionBar.Button("Reload");
    if (reloadButton.clicked) {
        Err err = map.Load(map.filename, now);
        if (err) {
            const char *filename = map.filename;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to reload file %s. %s\n", filename, ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState mapPath = uiActionBar.Text(GetFileName(map.filename), BROWN);
    if (mapPath.clicked) {
        system("explorer maps");
    }

    uiActionBar.Space({ 16, 0 });

    for (int i = 0; i < EditMode_Count; i++) {
        if (uiActionBar.Button(EditModeStr((EditMode)i), mode == i, DARKBLUE).pressed) {
            mode = (EditMode)i;
        }
    }

    uiActionBar.Space({ 16, 0 });

    UIState showCollidersButton = uiActionBar.Button("Show Colliders", state.showColliders ? RED : GRAY);
    if (showCollidersButton.clicked) {
        state.showColliders = !state.showColliders;
    }

    uiActionBar.Newline();

    switch (mode) {
        case EditMode_Tiles: {
            DrawUI_TileActions(io, uiActionBar, map);
            break;
        }
        case EditMode_Paths: {
            DrawUI_PathActions(io, uiActionBar);
            break;
        }
    }

    return uiState;
}

void Editor::DrawUI_TileActions(IO &io, UI &uiActionBar, Tilemap &map)
{
    uiActionBar.Text("Flags:", BROWN);
    UIState editCollisionButton = uiActionBar.Button("Collide", state.tiles.editCollision ? RED : GRAY);
    if (editCollisionButton.clicked) {
        state.tiles.editCollision = !state.tiles.editCollision;
    }

    uiActionBar.Newline();

    // [Editor] Tile selector
    const bool editorPickTileDef = io.MouseButtonPressed(MOUSE_BUTTON_LEFT);

    // TODO(dlb): UI::Panel (or row.style.colorBg?)
    Vector2 uiCursor = uiActionBar.CursorScreen();
    uiCursor = Vector2Add(uiCursor, uiActionBar.GetStyle().margin.TopLeft());

    DrawRectangle(
        uiCursor.x,
        uiCursor.y,
        map.tileDefCount * TILE_W + map.tileDefCount * 2 + 2,
        TILE_W + 4,
        BLACK
    );

    for (uint32_t i = 0; i < map.tileDefCount; i++) {
        TileDef &tileDef = map.tileDefs[i];
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
                if (state.tiles.editCollision) {
                    tileDef.collide = !tileDef.collide;
                } else {
                    state.tiles.cursor.tileDefId = i;
                }
            } else if (i != prevTileDefHovered) {
                PlaySound(sndSoftTick);
            }
            prevTileDefHovered = i;
        }

        DrawTextureRec(map.texture, texRect, screenPos, WHITE);

        if (state.tiles.editCollision && tileDef.collide) {
            const int pad = 1;
            DrawRectangleLinesEx(
                {
                    screenPos.x + pad,
                    screenPos.y + pad,
                    TILE_W - pad * 2,
                    TILE_W - pad * 2,
                },
                2.0f,
                MAROON
            );
        }

        const int outlinePad = 1;
        if (i == state.tiles.cursor.tileDefId || hover) {
            DrawRectangleLinesEx(
                {
                    screenPos.x - outlinePad,
                    screenPos.y - outlinePad,
                    TILE_W + outlinePad * 2,
                    TILE_W + outlinePad * 2
                },
                2,
                i == state.tiles.cursor.tileDefId ? YELLOW : WHITE
            );
        }
    }
}

void Editor::DrawUI_PathActions(IO &io, UI &uiActionBar)
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