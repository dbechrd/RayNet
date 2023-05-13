#include "audio/audio.h"
#include "collision.h"
#include "editor.h"
#include "texture_catalog.h"
#include "tilemap.h"
#include "tinyfiledialogs.h"
#include "ui/ui.h"

const char *EditModeStr(EditMode mode)
{
    switch (mode) {
        case EditMode_Tiles: return "Tiles";
        case EditMode_Paths: return "Paths";
        case EditMode_Warps: return "Warps";
        default: return "<null>";
    }
}

void Editor::HandleInput(Camera2D &camera)
{
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
}

void Editor::DrawOverlays(Tilemap &map, Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorOverlay);

    HandleInput(camera);

    // Draw tile collision layer
    if (state.showColliders) {
        map.DrawColliders(camera);
    }

    if (active) {
        switch (mode) {
            case EditMode_Tiles: {
                DrawOverlay_Tiles(map, camera, now);
                break;
            }
            case EditMode_Paths: {
                DrawOverlay_Paths(map, camera);
                break;
            }
            case EditMode_Warps: {
                DrawOverlay_Warps(map, camera);
                break;
            }
        }
    }

    io.PopScope();
}

void Editor::DrawOverlay_Tiles(Tilemap &map, Camera2D &camera, double now)
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

void Editor::DrawOverlay_Paths(Tilemap &map, Camera2D &camera)
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

void Editor::DrawOverlay_Warps(Tilemap &map, Camera2D &camera)
{
    for (const Warp &warp : map.warps) {
        DrawRectangleRec(warp.collider, Fade(SKYBLUE, 0.7f));
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
    const Rectangle actionBarRect{ position.x, position.y, 840, 160 };
#if 0
    DrawRectangleRounded(actionBarRect, 0.15f, 8, ASESPRITE_BEIGE);
    DrawRectangleRoundedLines(actionBarRect, 0.15f, 8, 2.0f, BLACK);
#else
    DrawRectangleRec(actionBarRect, Fade(ASESPRITE_BEIGE, 0.7f));
    DrawRectangleLinesEx(actionBarRect, 2.0f, BLACK);
#endif
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);

    uiActionBar.Text(TextFormat("v%d", map.version));
    UIState mapPath = uiActionBar.Text(GetFileName(map.filename.c_str()), WHITE);
    if (mapPath.clicked) {
        system("explorer maps");
    }
    uiActionBar.Newline();

    const char *mapFileFilter[1] = { "*.dat" };
    static std::string openRequest;
    static std::string saveAsRequest;

    UIState openButton = uiActionBar.Button("Open");
    if (openButton.clicked) {
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
    if (saveButton.clicked) {
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
    if (saveAsButton.clicked) {
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
    if (reloadButton.clicked) {
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

    uiActionBar.Text("--------------------------------------------------------------");
    uiActionBar.Newline();

    switch (mode) {
        case EditMode_Tiles: {
            DrawUI_TileActions(uiActionBar, map, now);
            DrawUI_Tilesheet(uiActionBar, map, now);
            break;
        }
        case EditMode_Paths: {
            DrawUI_PathActions(uiActionBar);
            break;
        }
        case EditMode_Warps: {
            DrawUI_WarpActions(uiActionBar);
            break;
        }
    }

    return uiState;
}

void Editor::DrawUI_TileActions(UI &uiActionBar, Tilemap &map, double now)
{
    const char *mapFileFilter[1] = { "*.png" };
    static const char *openRequest = 0;

    if (uiActionBar.Button("Change tileset").clicked) {
        std::string filename = rnTextureCatalog.GetEntry(map.textureId).path;
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
        Err err = Tilemap::ChangeTileset(map, openRequest, now);
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

    uiActionBar.Text("Flags:", BLACK);
    UIState editCollisionButton = uiActionBar.Button("Collide", state.tiles.editCollision ? RED : GRAY);
    if (editCollisionButton.clicked) {
        state.tiles.editCollision = !state.tiles.editCollision;
    }
}

void Editor::DrawUI_Tilesheet(UI &uiActionBar, Tilemap &map, double now)
{
    // TODO: Support multi-select (big rectangle?), and figure out where this lives

    Texture mapTex = rnTextureCatalog.GetTexture(map.textureId);
    Vector2 uiSheetPos{
        GetScreenWidth() - mapTex.width - 10.0f,
        GetScreenHeight() - mapTex.height - 10.0f
    };
    UIStyle uiSheetStyle{};
    UI uiSheet{ uiSheetPos, uiSheetStyle };

    static Texture checkerboard{};
    if (checkerboard.width != mapTex.width || checkerboard.height != mapTex.height) {
        checkerboard = LoadTextureFromImage(
            GenImageChecked(mapTex.width, mapTex.height, 8, 8, GRAY, LIGHTGRAY)
        );
    }

    UIState sheet = uiSheet.Image(checkerboard);
    DrawTextureEx(mapTex, sheet.contentTopLeft, 0, 1, WHITE);

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

    // Draw highlight around currently selected tiledef in draw mode
    if (state.tiles.cursor.tileDefId >= 0) {
        Rectangle tileDefRectScreen = map.TileDefRect(state.tiles.cursor.tileDefId);
        tileDefRectScreen.x += imgTL.x;
        tileDefRectScreen.y += imgTL.y;
        DrawRectangleLinesEx(tileDefRectScreen, 2, WHITE);
    }
}

void Editor::DrawUI_PathActions(UI &uiActionBar)
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

void Editor::DrawUI_WarpActions(UI &uiActionBar)
{
    uiActionBar.Text("TODO: Warp actions");
}