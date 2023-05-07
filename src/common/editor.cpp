#include "editor.h"
#include "ui/ui.h"

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
    if (io.IsKeyPressed(KEY_GRAVE)) {
        active = !active;
    }

    if (active) {
        if (io.IsKeyDown(KEY_LEFT_CONTROL) && io.IsKeyPressed(KEY_ZERO)) {
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
    if (!io.IsMouseCaptured()) {
        // Draw hover highlight
        const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);
        const bool editorPlaceTile = io.IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        const bool editorPickTile = io.IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        const bool editorFillTile = io.IsKeyPressed(KEY_F);

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

            bool hover = !io.IsMouseCaptured() && dlb_CheckCollisionPointRec(cursorWorldPos, nodeRect);
            if (hover) {
                io.CaptureMouse();

                color = aiPathNode->waitFor ? SKYBLUE : PINK;

                if (io.IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    cursor.dragging = true;
                    cursor.dragPathId = aiPathId;
                    cursor.dragPathNodeIndex = aiPathNodeIndex;
                    cursor.dragStartPosition = aiPathNode->pos;
                }
            }

            if (cursor.dragging && cursor.dragPathNodeIndex == aiPathNodeIndex) {
                io.PushScope(IO::IO_EditorDrag);
                if (io.IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    io.CaptureMouse();
                    io.CaptureKeyboard();

                    color = aiPathNode->waitFor ? DARKBLUE : MAROON;

                    if (io.IsKeyPressed(KEY_ESCAPE)) {
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
        if (io.IsKeyDown(KEY_LEFT_SHIFT)) {
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

    UIState saveButton = uiActionBar.Button("Save");
    if (saveButton.clicked) {
        if (map.Save(LEVEL_001) != RN_SUCCESS) {
            // TODO: Display error message on screen for N seconds or
            // until dismissed
        }
    }

    UIState loadButton = uiActionBar.Button("Load");
    if (loadButton.clicked) {
        Err err = map.Load(LEVEL_001, now);
        if (err != RN_SUCCESS) {
            printf("Failed to load map with code %d\n", err);
            assert(!"oops");
            // TODO: Display error message on screen for N seconds or
            // until dismissed
        }
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
    uiActionBar.Text("Flags:", BLACK);
    UIState editCollisionButton = uiActionBar.Button("Collide", state.tiles.editCollision ? RED : GRAY);
    if (editCollisionButton.clicked) {
        state.tiles.editCollision = !state.tiles.editCollision;
    }

    uiActionBar.Newline();

    // [Editor] Tile selector
    const bool editorPickTileDef = io.IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

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