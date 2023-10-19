#include "../common/collision.h"
#include "../common/common.h"
#include "../common/data.h"
#include "../common/entity_db.h"
#include "../common/io.h"
#include "../common/texture_catalog.h"
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
        case EditMode_Warps:     return "Warps";
        case EditMode_Dialog:    return "Dialog";
        case EditMode_Entities:  return "Entities";
        case EditMode_SfxFiles:  return "Sfx";
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

    auto &map = data::packs[0]->FindTilemap(map_id);

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
        auto &map = data::packs[0]->FindTilemap(map_id);

        if (map.AtWorld((int32_t)cursorWorldPos.x, (int32_t)cursorWorldPos.y, hoveredTile)) {
            data::Tilemap::Coord coord{};
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
void Editor::DrawGroundOverlay_Wang(Camera2D &camera, double now)
{
}
void Editor::DrawGroundOverlay_Paths(Camera2D &camera, double now)
{
    auto &cursor = state.pathNodes.cursor;
    const Vector2 cursorWorldPos = GetScreenToWorld2D({ (float)GetMouseX(), (float)GetMouseY() }, camera);

    // Draw path edges
    auto &map = data::packs[0]->FindTilemap(map_id);
    for (uint32_t aiPathId = 0; aiPathId < map.paths.size(); aiPathId++) {
        data::AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = aiPath->pathNodeStart; aiPathNodeIndex < aiPath->pathNodeStart + aiPath->pathNodeCount; aiPathNodeIndex++) {
            uint32_t aiPathNodeNextIndex = map.GetNextPathNodeIndex(aiPathId, aiPathNodeIndex);
            data::AiPathNode *aiPathNode = map.GetPathNode(aiPathId, aiPathNodeIndex);
            data::AiPathNode *aiPathNodeNext = map.GetPathNode(aiPathId, aiPathNodeNextIndex);
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
        data::AiPath *aiPath = map.GetPath(aiPathId);
        if (!aiPath) {
            continue;
        }

        for (uint32_t aiPathNodeIndex = aiPath->pathNodeStart; aiPathNodeIndex < aiPath->pathNodeStart + aiPath->pathNodeCount; aiPathNodeIndex++) {
            data::AiPathNode *aiPathNode = map.GetPathNode(aiPathId, aiPathNodeIndex);
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
        data::AiPath *aiPath = map.GetPath(cursor.dragPathId);
        if (aiPath) {
            data::AiPathNode *aiPathNode = map.GetPathNode(cursor.dragPathId, cursor.dragPathNodeIndex);
            assert(aiPathNode);
            aiPathNode->pos.x = newNodePos.x;
            aiPathNode->pos.y = newNodePos.y;
        }

        io.PopScope();
    }
}
void Editor::DrawGroundOverlay_Warps(Camera2D &camera, double now)
{
    for (const data::Entity &entity : data::packs[0]->entities) {
        if (entity.type == data::ENTITY_SPEC_OBJ_WARP) {
            DrawRectangleRec(entity.warp_collider, Fade(SKYBLUE, 0.7f));
        }
    }
}

void Editor::DrawEntityOverlays(Camera2D &camera, double now)
{
    io.PushScope(IO::IO_EditorEntityOverlay);

    data::Entity *selectedEntity = entityDb->FindEntity(state.entities.selectedId);
    auto &map = data::packs[0]->FindTilemap(map_id);
    if (selectedEntity && selectedEntity->map_id == map.id) {
        DrawTextEx(fntSmall, TextFormat("[selected in editor]\n%u", selectedEntity->id),
            selectedEntity->ScreenPos(), fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
    }

    if (state.showEntityIds) {
        for (data::Entity &entity : entityDb->entities) {
            if (entity.map_id == map.id && entity.id != state.entities.selectedId) {
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
    auto &map = data::packs[0]->FindTilemap(map_id);
    for (data::Entity &entity : entityDb->entities) {
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
            if (mouseWheel < 0) impulse *= -1;
            scrollVelocity += impulse;
            //printf("wheel: %f, target: %f\n", mouseWheel, scrollOffsetTarget);
        } else {
            scrollPanel.scrollAccel = 0;
        }
    }

    if (scrollVelocity) {
        scrollOffsetTarget -= scrollVelocity;
        scrollVelocity *= 0.95f;
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
        scrollPanel.panelRect.width - 2 - scrollbarWidth,
        scrollPanel.panelRect.y + scrollbarSpace * scrollPct,
        scrollbarWidth,
        scrollbarHeight
    };
    DrawRectangleRec(scrollbar, LIGHTGRAY);
    DrawRectangleLinesEx(scrollPanel.panelRect, 2, BLACK);
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
    //DrawRectangleRec(actionBarRect, ColorBrightness(ASESPRITE_BEIGE, -0.1f));
    DrawRectangleRec(actionBarRect, GRAYISH_BLUE);
    DrawRectangleLinesEx(actionBarRect, 2.0f, BLACK);
#endif
    uiState.hover = dlb_CheckCollisionPointRec(GetMousePosition(), actionBarRect);

    const char *mapFileFilter[1] = { "*.mdesk" };
    static std::string openRequest;
    static std::string saveAsRequest;

    auto &map = data::packs[0]->FindTilemap(map_id);

    UIState openButton = uiActionBar.Button("Open");
    if (openButton.released) {
        std::string filename = map.id;
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
        data::Tilemap *openedMap = server.FindOrLoadMap(openRequest);
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
        std::string filename = "resources/map/" + map.id + ".mdesk";
        Err err = data::SaveTilemap(filename, map);
        if (err) {
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to save file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }

    UIState saveAsButton = uiActionBar.Button("Save As");
    if (saveAsButton.released) {
        std::string filename = "resources/map/" + map.id + ".mdesk";
        std::thread saveAsThread([filename, mapFileFilter]{
            const char *saveAsRequestBuf = tinyfd_saveFileDialog(
                "Save File",
                filename.c_str(),
                ARRAY_SIZE(mapFileFilter),
                mapFileFilter,
                "RayNet Tilemap (*.mdesk)");
            if (saveAsRequestBuf) saveAsRequest = saveAsRequestBuf;
        });
        saveAsThread.detach();
    }
    if (saveAsRequest.size()) {
        Err err = data::SaveTilemap(saveAsRequest, map);
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
        assert(!"not implemented with new map format");
        Err err = RN_BAD_FILE_READ; // map.Load(map.id);
        if (err) {
            std::string filename = map.id;
            std::thread errorThread([filename, err]{
                const char *msg = TextFormat("Failed to reload file %s. %s\n", filename.c_str(), ErrStr(err));
                tinyfd_messageBox("Error", msg, "ok", "error", 1);
            });
            errorThread.detach();
        }
    }
    uiActionBar.Newline();

    UIState mapPath = uiActionBar.Text(GetFileName(map.id.c_str()), WHITE);
    if (mapPath.released) {
        system("explorer maps");
    }
    uiActionBar.Text(TextFormat("(v%d)", map.version));
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
        if (uiActionBar.Button(EditModeStr((EditMode)i), mode == i, BLUE_DESAT, ColorBrightness(BLUE_DESAT, -0.3f)).pressed) {
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
        case EditMode_Dialog: {
            DrawUI_DialogActions(uiActionBar, now);
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
        case EditMode_Debug: {
            DrawUI_Debug(uiActionBar, now);
            break;
        }
    }

    return uiState;
}
void Editor::DrawUI_MapActions(UI &uiActionBar, GameServer &server, double now)
{
    for (const data::Tilemap &map : data::packs[0]->tile_maps) {
        if (uiActionBar.Button(TextFormat("[%d] %s", map.net_id, map.id.c_str())).pressed) {
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
    std::string tilesetPath = rnStringCatalog.GetString(map.textureId);
    uiActionBar.Text(tilesetPath.c_str());
    if (uiActionBar.Button("Change tileset", ColorBrightness(ORANGE, -0.3f)).released) {
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
        Err err = data::Tilemap::ChangeTileset(*map, openRequest, now);
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
    // TODO(dlb): Support multi-select (big rectangle?), and figure out where this lives
    auto &map = data::packs[0]->FindTilemap(map_id);

    size_t tile_def_count = map.tileDefs.size();
    int tiles_x = 6;
    int tiles_y = tile_def_count / 6 + (tile_def_count % 6 > 0);
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
    for (int i = 0; i < map.tileDefs.size(); i++) {
        int tile_x = TILE_W * (i % tiles_x);
        int tile_y = TILE_W * (i / tiles_x);
        map.DrawTile(i, { imgTL.x + tile_x, imgTL.y + tile_y });
    }

    uiActionBar.PopStyle();

    // Draw collision overlay on tilesheet if we're in collision editing mode
    switch (state.tiles.tileEditMode) {
        case TileEditMode_Collision: {
            for (int i = 0; i < map.tileDefs.size(); i++) {
                const data::Material &material = data::packs[0]->FindMaterial(map.tileDefs[i].material);
                const float tileThird = TILE_W / 3;
                const float tileSixth = tileThird / 2.0f;
                Vector2 cursor{
                    imgTL.x + TILE_W * (i % tiles_x),
                    imgTL.y + TILE_W * (i / tiles_x)
                };

                cursor.x++;
                cursor.y++;

                // TODO: Make this a loop + LUT if we start adding more flags
                if (!material.flags) {
                    DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(MAROON, 0.7f));

                    Vector2 size = dlb_MeasureTextEx(fntSmall, "X", 1, 0);
                    Vector2 pos = cursor;
                    pos.x += tileSixth - size.x / 2.0f;
                    pos.y += tileSixth - size.y / 2.0f;
                    DrawTextShadowEx(fntSmall, "X", pos, WHITE);
                } else {
                    if (material.flags & data::MATERIAL_FLAG_WALK) {
                        DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(DARKGREEN, 0.7f));

                        Vector2 size = dlb_MeasureTextEx(fntSmall, "W", 1, 0);
                        Vector2 pos = cursor;
                        pos.x += tileSixth - size.x / 2.0f;
                        pos.y += tileSixth - size.y / 2.0f;
                        DrawTextShadowEx(fntSmall, "W", pos, WHITE);
                    }
                    cursor.x += tileThird + 1;
                    if (material.flags & data::MATERIAL_FLAG_SWIM) {
                        DrawRectangle(cursor.x, cursor.y, tileThird, tileThird, Fade(SKYBLUE, 0.7f));

                        Vector2 size = dlb_MeasureTextEx(fntSmall, "S", 1, 0);
                        Vector2 pos = cursor;
                        pos.x += tileSixth - size.x / 2.0f;
                        pos.y += tileSixth - size.y / 2.0f;
                        DrawTextShadowEx(fntSmall, "S", pos, WHITE);
                    }
                    cursor.x += tileThird + 1;
                }
            }
            break;
        }
        case TileEditMode_AutoTileMask: {
            for (int i = 0; i < map.tileDefs.size(); i++) {
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
                uint8_t mask = map.tileDefs[i].auto_tile_mask;
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

        // Draw hover highlight on tilesheet if mouse hovering a valid tiledef
        const int tile_x = (int)mouseRel.x / TILE_W;
        const int tile_y = (int)mouseRel.y / TILE_W;
        const int tileIdx = tile_y * tiles_x + tile_x;

        if (tileIdx < tile_def_count) {
            if (state.tiles.tileEditMode != TileEditMode_AutoTileMask) {
                DrawRectangleLinesEx(
                    { imgTL.x + tile_x * TILE_W, imgTL.y + tile_y * TILE_W, TILE_W, TILE_W },
                    2,
                    Fade(WHITE, 0.7f)
                );
            }
        }

        //DrawTextEx(fntHackBold20, TextFormat("%d, %d\n", tileX, tileY),
        //    Vector2Add(GetMousePosition(), { 10, 10 }), fntHackBold20.baseSize, 1, YELLOW);

        // If mouse pressed, select tile, or change collision data, depending on mode
        if (sheet.pressed) {
            if (tileIdx >= 0 && tileIdx < map.tileDefs.size()) {
                switch (state.tiles.tileEditMode) {
                    case TileEditMode_Select: {
                        state.tiles.cursor.tileDefId = tileIdx;
                        break;
                    }
                    case TileEditMode_Collision:
                    case TileEditMode_AutoTileMask: {
                        data::TileDef &tileDef = map.tileDefs[tileIdx];
                        
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
                            data::Material& mat = data::packs[0]->FindMaterial(tileDef.material);
                            if (!mat.id.empty()) {
                                switch (tileSegment) {
                                    case 0: mat.flags ^= data::MATERIAL_FLAG_WALK; break;
                                    case 1: mat.flags ^= data::MATERIAL_FLAG_SWIM; break;
                                }
                            }
                        } else if (state.tiles.tileEditMode == TileEditMode_AutoTileMask) {
                            //printf("x: %d, y: %d, s: %d\n", tileXSegment, tileYSegment, tileSegment);
                            switch (tileSegment) {
                                case 0: tileDef.auto_tile_mask ^= 0b10000000; break;
                                case 1: tileDef.auto_tile_mask ^= 0b01000000; break;
                                case 2: tileDef.auto_tile_mask ^= 0b00100000; break;
                                case 3: tileDef.auto_tile_mask ^= 0b00010000; break;
                                case 4: tileDef.auto_tile_mask ^= 0b00000000; break;
                                case 5: tileDef.auto_tile_mask ^= 0b00001000; break;
                                case 6: tileDef.auto_tile_mask ^= 0b00000100; break;
                                case 7: tileDef.auto_tile_mask ^= 0b00000010; break;
                                case 8: tileDef.auto_tile_mask ^= 0b00000001; break;
                            }
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
        int tile_x = state.tiles.cursor.tileDefId % tiles_x;
        int tile_y = state.tiles.cursor.tileDefId / tiles_x;
        Rectangle tileDefRectScreen{
            imgTL.x + (TILE_W * tile_x),
            imgTL.y + (TILE_W * tile_y),
            TILE_W,
            TILE_W
        };
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

    auto &map = data::packs[0]->FindTilemap(map_id);

    if (uiActionBar.Button("Re-generate Map").pressed) {
        wangTileset.GenerateMap(map.width, map.height, wangMap);
    }
    uiActionBar.Newline();

    static data::Tilemap wangTilemap{};
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

        uint8_t *imgData = (uint8_t *)wangTileset.ts.img.data;

        if (hTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.h_tiles[hTex];
            for (int y = 0; y < wangTileset.ts.short_side_len; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len*2; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < map.tileDefs.size() ? pixel[0] : 0;

                    const data::GfxFrame &gfx_frame = map.GetTileGfxFrame(selectedTile);
                    const data::GfxFile &gfx_file = data::packs[0]->FindGraphic(gfx_frame.gfx);
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
        } else if (vTex >= 0) {
            stbhw_tile *wangTile = wangTileset.ts.v_tiles[vTex];
            for (int y = 0; y < wangTileset.ts.short_side_len*2; y++) {
                for (int x = 0; x < wangTileset.ts.short_side_len; x++) {
                    int templateX = wangTile->x + x;
                    int templateY = wangTile->y + y;
                    int templateOffset = (templateY * wangTileset.ts.img.width + templateX) * 3;
                    uint8_t *pixel = &imgData[templateOffset];
                    uint8_t tile = pixel[0] < map.tileDefs.size() ? pixel[0] : 0;

                    const data::GfxFrame &gfx_frame = map.GetTileGfxFrame(selectedTile);
                    const data::GfxFile &gfx_file = data::packs[0]->FindGraphic(gfx_frame.gfx);
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
    //    map.warps.clear();
    //}
    //uiActionBar.Newline();

    if (uiActionBar.Button("Add", DARKGREEN).pressed) {
        //data::Tilemap &map = data::packs[0]->
        //map.warps.push_back({});
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

    //for (data::AspectWarp &warp : data::packs[0]->warp) {
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

    for (data::Entity &entity : data::packs[0]->entities) {
        if (entity.spec != data::ENTITY_SPEC_OBJ_WARP) {
            continue;
        }

        uiActionBar.Text("collider");
        uiActionBar.Newline();

        uiActionBar.PushWidth(80);

        uiActionBar.Text("x");
        uiActionBar.Text("y");
        uiActionBar.Text("width");
        uiActionBar.Text("height");
        uiActionBar.Newline();

        static STB_TexteditState txtColliderX{};
        uiActionBar.TextboxFloat(txtColliderX, entity.warp_collider.x);
        static STB_TexteditState txtColliderY{};
        uiActionBar.TextboxFloat(txtColliderY, entity.warp_collider.y);
        static STB_TexteditState txtColliderW{};
        uiActionBar.TextboxFloat(txtColliderW, entity.warp_collider.width);
        static STB_TexteditState txtColliderH{};
        uiActionBar.TextboxFloat(txtColliderH, entity.warp_collider.height);
        uiActionBar.Newline();

        uiActionBar.Text("destX");
        uiActionBar.Text("destY");
        uiActionBar.Newline();

        static STB_TexteditState txtDestX{};
        uiActionBar.TextboxFloat(txtDestX, entity.warp_dest_pos.x);
        static STB_TexteditState txtDestY{};
        uiActionBar.TextboxFloat(txtDestY, entity.warp_dest_pos.y);
        uiActionBar.Newline();

        uiActionBar.PopStyle();

        uiActionBar.Text("destMap");
        uiActionBar.Newline();
        static STB_TexteditState txtDestMap{};
        uiActionBar.Textbox(txtDestMap, entity.warp_dest_map);
        uiActionBar.Newline();

        uiActionBar.Text("templateMap");
        uiActionBar.Newline();
        static STB_TexteditState txtTemplateMap{};
        uiActionBar.Textbox(txtTemplateMap, entity.warp_template_map);
        uiActionBar.Newline();

        uiActionBar.Text("templateTileset");
        uiActionBar.Newline();
        static STB_TexteditState txtTemplateTileset{};
        uiActionBar.Textbox(txtTemplateTileset, entity.warp_template_tileset);
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();
}
void Editor::DrawUI_DialogActions(UI &uiActionBar, double now)
{
#if 0
    //if (uiActionBar.Button("Add", DARKGREEN).pressed) {
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
            uiActionBar.Text("message");
            uiActionBar.Newline();
            static STB_TexteditState txtMessage{};
            uiActionBar.Textbox(txtMessage, dialog.msg);
            uiActionBar.Newline();

            static STB_TexteditState txtOptionId[SV_MAX_ENTITY_DIALOG_OPTIONS]{};
            for (int i = 0; i < SV_MAX_ENTITY_DIALOG_OPTIONS; i++) {
                uiActionBar.Text(TextFormat("option %d", i));
                uiActionBar.Newline();
                float id = dialog.option_ids[i];
                uiActionBar.TextboxFloat(txtOptionId[i], id, 0, "%.f");
                dialog.option_ids[i] = (data::DialogId)CLAMP(id, 0, data::packs[0]->dialogs.size() - 1);
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
    auto &map = data::packs[0]->FindTilemap(map_id);
    if (uiActionBar.Button("Despawn all", ColorBrightness(MAROON, -0.3f)).pressed) {
        for (const data::Entity &entity : entityDb->entities) {
            if (entity.type == data::ENTITY_PLAYER || entity.map_id != map.id) {
                continue;
            }
            assert(entity.id && entity.type);
            entityDb->DespawnEntity(entity.id, now);
        }
    }

    if (uiActionBar.Button(TextFormat("Despawn Test %d", state.entities.testId), ColorBrightness(MAROON, -0.3f)).pressed) {
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
        if (!entity.id || entity.map_id != map.id) {
            continue;
        }

        const char *idStr = TextFormat("[%d] %s", entity.id, EntityTypeStr(entity.type));
        if (!StrFilter(idStr, filter.c_str())) {
            continue;
        }

        Color bgColor = entity.id == state.entities.selectedId ? ColorBrightness(ORANGE, -0.3f) : BLUE_DESAT;
        if (uiActionBar.Text(idStr, WHITE, bgColor).down) {
            state.entities.selectedId = entity.id;
        }
        uiActionBar.Newline();
    }

    uiActionBar.PopStyle();

    if (state.entities.selectedId) {
        data::Entity *entity = entityDb->FindEntity(state.entities.selectedId);
        if (entity) {
            const int labelWidth = 100;

            ////////////////////////////////////////////////////////////////////////
            // Entity
            uiActionBar.Label("id", labelWidth);
            uiActionBar.Text(TextFormat("%d", entity->id));
            uiActionBar.Newline();

            uiActionBar.Label("type", labelWidth);
            uiActionBar.Text(EntityTypeStr(entity->type));
            uiActionBar.Newline();

            uiActionBar.Label("position", labelWidth);
            uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeDefault);
            static STB_TexteditState txtPosX{};
            uiActionBar.TextboxFloat(txtPosX, entity->position.x, 80);
            uiActionBar.PopStyle();
            uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeDefault);
            static STB_TexteditState txtPosY{};
            uiActionBar.TextboxFloat(txtPosY, entity->position.y, 80);
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Combat
            uiActionBar.Label("attk cooldown", labelWidth);
            const float attackCooldownLeft = MAX(0, entity->attack_cooldown - (now - entity->last_attacked_at));
            uiActionBar.Text(TextFormat("%.3f", attackCooldownLeft));
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Collision
            uiActionBar.Label("radius", labelWidth);
            static STB_TexteditState txtRadius{};
            uiActionBar.TextboxFloat(txtRadius, entity->radius, 80);
            uiActionBar.Newline();

            uiActionBar.Label("colliding", labelWidth);
            if (entity->colliding) {
                uiActionBar.Text("True", RED);
            } else {
                uiActionBar.Text("False", WHITE);
            }
            uiActionBar.Newline();

            uiActionBar.Label("onWarp", labelWidth);
            if (entity->on_warp) {
                uiActionBar.Text("True", SKYBLUE);
            } else {
                uiActionBar.Text("False", WHITE);
            }
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Life
            uiActionBar.Label("health", labelWidth);
            static STB_TexteditState txtHealth{};
            uiActionBar.TextboxFloat(txtHealth, entity->hp, 80);
            uiActionBar.Text("/");
            static STB_TexteditState txtMaxHealth{};
            uiActionBar.TextboxFloat(txtMaxHealth, entity->hp_max, 80);
            uiActionBar.Newline();

            ////////////////////////////////////////////////////////////////////////
            // Physics
            uiActionBar.Label("drag", labelWidth);
            static STB_TexteditState txtDrag{};
            uiActionBar.TextboxFloat(txtDrag, entity->drag, 80);
            uiActionBar.Newline();

            uiActionBar.Label("speed", labelWidth);
            static STB_TexteditState txtSpeed{};
            uiActionBar.TextboxFloat(txtSpeed, entity->speed, 80);
            uiActionBar.Newline();

            uiActionBar.Label("velocity", labelWidth);
            uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeDefault);
            static STB_TexteditState txtVelX{};
            uiActionBar.TextboxFloat(txtVelX, entity->velocity.x, 80);
            uiActionBar.PopStyle();
            uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeDefault);
            static STB_TexteditState txtVelY{};
            uiActionBar.TextboxFloat(txtVelY, entity->velocity.y, 80);
            uiActionBar.PopStyle();
            uiActionBar.Newline();
        } else {
            state.entities.selectedId = 0;
        }
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

    for (data::SfxFile &sfxFile : data::packs[0]->sfx_files) {
        if (sfxFile.id.empty()) {
            continue;
        }

        Color bgColor = sfxFile.id == state.sfxFiles.selectedSfx ? SKYBLUE : BLUE_DESAT;
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

    if (!state.sfxFiles.selectedSfx.empty()) {
        const int labelWidth = 100;
        data::SfxFile &sfx_file = data::packs[0]->FindSoundVariant(state.sfxFiles.selectedSfx);
        if (!sfx_file.id.empty()) {
            uiActionBar.Label("id", labelWidth);
            uiActionBar.Text(TextFormat("%d", sfx_file.id));
            uiActionBar.Newline();

            uiActionBar.Label("path", labelWidth);
            static STB_TexteditState txtPath{};
            uiActionBar.Textbox(txtPath, sfx_file.path);
            uiActionBar.Newline();

            uiActionBar.Label("pitch variance", labelWidth);
            static STB_TexteditState txtPitchVariance{};
            uiActionBar.TextboxFloat(txtPitchVariance, sfx_file.pitch_variance);
            uiActionBar.Newline();
        } else {
            state.sfxFiles.selectedSfx.clear();
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

        Color bgColor = packPtr == state.packFiles.selectedPack ? SKYBLUE : BLUE_DESAT;
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

        static struct DatTypeFilter {
            bool enabled;
            const char *text;
            Color color;
        } datTypeFilter[data::DAT_TYP_COUNT]{
            { true,  "GFX", ColorFromHSV(0 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MUS", ColorFromHSV(1 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "SFX", ColorFromHSV(2 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "FRM", ColorFromHSV(3 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "ANM", ColorFromHSV(4 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MAT", ColorFromHSV(5 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "SPT", ColorFromHSV(6 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "MAP", ColorFromHSV(7 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
            { true,  "ENT", ColorFromHSV(8 * (360.0f / (float)data::DAT_TYP_COUNT), 0.9f, 0.6f) },
        };

        uiActionBar.PushWidth(34);
        for (int i = 1; i < data::DAT_TYP_COUNT; i++) {
            DatTypeFilter &filter = datTypeFilter[i];
            if (uiActionBar.Button(filter.text ? filter.text : "???", filter.enabled, DARKGRAY, filter.color).pressed) {
                if (io.KeyDown(KEY_LEFT_SHIFT)) {
                    filter.enabled = !filter.enabled;
                } else {
                    for (int j = 0; j < data::DAT_TYP_COUNT; j++) {
                        datTypeFilter[j].enabled = false;
                    }
                    filter.enabled = true;
                }
            }
            if ((i && i % 7 == 0) || i == data::DAT_TYP_COUNT - 1) {
                uiActionBar.Newline();
            }
        }
        uiActionBar.PopStyle();

        uiActionBar.Space({ 0, 4 });

        static ScrollPanel scrollPanel{};
        BeginScrollPanel(uiActionBar, scrollPanel, 430);

        // Defer changing selection until after the loop has rendered every item
        int newSelectedOffset = 0;

        for (data::PackTocEntry &entry : pack.toc.entries) {
            DatTypeFilter &filter = datTypeFilter[entry.dtype];
            if (!filter.enabled) {
                continue;
            }

            bool selected = state.packFiles.selectedPackEntryOffset == entry.offset;

            const char *desc = DataTypeStr(entry.dtype);

            switch (entry.dtype) {
                case data::DAT_TYP_GFX_FILE:
                {
                    data::GfxFile &gfx_file = pack.gfx_files[entry.index];
                    desc = gfx_file.path.c_str();
                    break;
                }
                case data::DAT_TYP_MUS_FILE:
                {
                    data::MusFile &mus_file = pack.mus_files[entry.index];
                    desc = mus_file.id.c_str();
                    break;
                }
                case data::DAT_TYP_SFX_FILE:
                {
                    data::SfxFile &sfx_file = pack.sfx_files[entry.index];
                    desc = sfx_file.id.c_str();
                    break;
                }
                case data::DAT_TYP_GFX_FRAME:
                {
                    data::GfxFrame &gfx_frame = pack.gfx_frames[entry.index];
                    desc = gfx_frame.id.c_str();
                    break;
                }
                case data::DAT_TYP_GFX_ANIM:
                {
                    data::GfxAnim &gfx_anim = pack.gfx_anims[entry.index];
                    desc = gfx_anim.id.c_str();
                    break;
                }
                case data::DAT_TYP_MATERIAL:
                {
                    data::Material &material = pack.materials[entry.index];
                    desc = material.id.c_str();
                    break;
                }
                case data::DAT_TYP_SPRITE:
                {
                    data::Sprite &sprite = pack.sprites[entry.index];
                    desc = sprite.id.c_str();
                    break;
                }
                case data::DAT_TYP_TILE_MAP:
                {
                    data::Tilemap &tile_map = pack.tile_maps[entry.index];
                    desc = tile_map.id.c_str();
                    break;
                }
                case data::DAT_TYP_ENTITY:
                {
                    data::Entity &entity = pack.entities[entry.index];
                    desc = data::EntityTypeStr(entity.type);
                    break;
                }
            }

            uiActionBar.PushWidth(400);
            uiActionBar.PushMargin({ 1, 0, 1, 0 });

            const char *text = TextFormat("[%s] %s", selected ? "-" : "+", desc);
            if (uiActionBar.Text(text, WHITE, ColorBrightness(filter.color, -0.2f)).pressed) {
                printf("pressed\n");
                if (!selected) {
                    newSelectedOffset = entry.offset;
                } else {
                    state.packFiles.selectedPackEntryOffset = 0;
                }
            }

            uiActionBar.PopStyle();
            uiActionBar.PopStyle();
            uiActionBar.Newline();

            int detailsLabelWidth = 80;

            if (selected) {
                switch (entry.dtype) {
                    case data::DAT_TYP_GFX_FILE:
                    {
                        data::GfxFile &gfxFile = pack.gfx_files[entry.index];
                        uiActionBar.Label("Path", detailsLabelWidth);
                        uiActionBar.Text(gfxFile.path.c_str());
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_MUS_FILE:
                    {
                        data::MusFile &musFile = pack.mus_files[entry.index];
                        uiActionBar.Label("Path", detailsLabelWidth);
                        uiActionBar.Text(musFile.path.c_str());
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_SFX_FILE:
                    {
                        static int labelWidth = 100;
                        const float textboxWidth = 400 - 16 - labelWidth;

                        data::SfxFile &sfxFile = pack.sfx_files[entry.index];

                        int newLabelWidth = 0;
                        UIState uiPath = uiActionBar.Label("path", labelWidth);
                        newLabelWidth = MAX(newLabelWidth, uiPath.contentRect.width);
                        static STB_TexteditState txtPath{};
                        uiActionBar.PushWidth(textboxWidth);
                        uiActionBar.Textbox(txtPath, sfxFile.path);
                        uiActionBar.PopStyle();
                        uiActionBar.Newline();

                        UIState uiPitch = uiActionBar.Label("pitch variance", labelWidth);
                        newLabelWidth = MAX(newLabelWidth, uiPitch.contentRect.width);
                        static STB_TexteditState txtPitchVariance{};
                        uiActionBar.TextboxFloat(txtPitchVariance, sfxFile.pitch_variance, textboxWidth, "%.2f", 0.01f);
                        uiActionBar.Newline();

                        if (!data::IsSoundPlaying(sfxFile.id)) {
                            if (uiActionBar.Button("Play", ColorBrightness(DARKGREEN, -0.3f)).pressed) {
                                data::PlaySound(sfxFile.id, 0);
                            }
                        } else {
                            if (uiActionBar.Button("Stop", ColorBrightness(MAROON, -0.3f)).pressed) {
                                data::StopSound(sfxFile.id);
                            }
                        }

                        uiActionBar.Newline();

                        labelWidth = newLabelWidth;
                        break;
                    }
                    case data::DAT_TYP_GFX_FRAME:
                    {
                        data::GfxFrame &gfxFrame = pack.gfx_frames[entry.index];
                        //uiActionBar.Label("rect", detailsLabelWidth);
                        //uiActionBar.Text(TextFormat("%hu", gfxFrame.x));
                        //uiActionBar.Text(TextFormat("%hu", gfxFrame.y));
                        //uiActionBar.Text(TextFormat("%hu", gfxFrame.w));
                        //uiActionBar.Text(TextFormat("%hu", gfxFrame.h));

                        uiActionBar.Label("rect", detailsLabelWidth);

                        uiActionBar.PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeDefault);
                        static STB_TexteditState txtX{};
                        float x = (float)gfxFrame.x;
                        uiActionBar.TextboxFloat(txtX, x, 40, "%.f");
                        gfxFrame.x = CLAMP(x, 0, UINT16_MAX);
                        uiActionBar.PopStyle();

                        uiActionBar.PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeDefault);
                        static STB_TexteditState txtY{};
                        float y = (float)gfxFrame.y;
                        uiActionBar.TextboxFloat(txtY, y, 40, "%.f");
                        gfxFrame.y = CLAMP(y, 0, UINT16_MAX);
                        uiActionBar.PopStyle();

                        static STB_TexteditState txtW{};
                        float w = (float)gfxFrame.w;
                        uiActionBar.TextboxFloat(txtW, w, 40, "%.f");
                        gfxFrame.w = CLAMP(w, 0, UINT16_MAX);

                        static STB_TexteditState txtH{};
                        float h = (float)gfxFrame.h;
                        uiActionBar.TextboxFloat(txtH, h, 40, "%.f");
                        gfxFrame.h = CLAMP(h, 0, UINT16_MAX);
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_GFX_ANIM:
                    {
                        data::GfxAnim &gfxAnim = pack.gfx_anims[entry.index];
                        uiActionBar.Label("id", detailsLabelWidth);
                        uiActionBar.Text(gfxAnim.id.c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("sound", detailsLabelWidth);
                        uiActionBar.Text(gfxAnim.sound.c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("frame_rate", detailsLabelWidth);
                        uiActionBar.Text(TextFormat("%u", gfxAnim.frame_rate));
                        uiActionBar.Newline();

                        uiActionBar.Label("frame_delay", detailsLabelWidth);
                        uiActionBar.Text(TextFormat("%u", gfxAnim.frame_delay));
                        uiActionBar.Newline();

                        uiActionBar.Label("frames", detailsLabelWidth);
                        uiActionBar.Newline();

                        for (int i = 0; i < gfxAnim.frame_count; i++) {
                            uiActionBar.Text(TextFormat("[%d] %s", i, gfxAnim.frames[i].c_str()));
                            uiActionBar.Newline();
                        }
                        break;
                    }
                    case data::DAT_TYP_MATERIAL:
                    {
                        data::Material &material = pack.materials[entry.index];

                        uiActionBar.Label("id", detailsLabelWidth);
                        uiActionBar.Text(material.id.c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("footstep", detailsLabelWidth);
                        uiActionBar.Text(material.footstep_sound.c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("flags", detailsLabelWidth);
                        uiActionBar.Button("walk", material.flags & data::MATERIAL_FLAG_WALK, DARKGRAY, BLUE);
                        uiActionBar.Button("swim", material.flags & data::MATERIAL_FLAG_SWIM, DARKGRAY, BLUE);
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_SPRITE:
                    {
                        data::Sprite &sprite = pack.sprites[entry.index];

                        const float labelWidth = 20.0f;
                        uiActionBar.Label("id", labelWidth);
                        uiActionBar.Text(sprite.id.c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("N ", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_N].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("E ", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_E].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("S ", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_S].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("S ", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_W].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("NE", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_SE].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("SE", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_SE].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("SW", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_SW].c_str());
                        uiActionBar.Newline();

                        uiActionBar.Label("NW", labelWidth);
                        uiActionBar.Text(sprite.anims[data::DIR_NW].c_str());
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_TILE_MAP:
                    {
                        uiActionBar.Text("TODO");
                        uiActionBar.Newline();
                        break;
                    }
                    case data::DAT_TYP_ENTITY:
                    {
                        uiActionBar.Text("TODO");
                        uiActionBar.Newline();
                        break;
                    }
                }
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
    uiActionBar.Text(TextFormat("%.*s", 20, uid.bytes));
    if (true) {// || uiActionBar.Button("NextUID").pressed) {
        uid = NextUID();
    }
    uiActionBar.Newline();

#if 0
    uiActionBar.Text("^^^ ignore all this stuff ^^^");
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

    if (uiActionBar.Button("Reset").pressed) {
        for (int i = 0; i < ARRAY_SIZE(elements); i++) {
            elements[i] = elements_default[i];
        }
    }
    uiActionBar.Newline();

    const bool *built = 0;
    bool multi = false;
    int selected_count = 0;

    uiActionBar.Text("_____________________________________________", GRAY);
    uiActionBar.Newline();
    uiActionBar.Text("Elements:", LIGHTGRAY);
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
        uiActionBar.Button("Various Statuses", true, PURPLE, PURPLE);
    }
    uiActionBar.Newline();

    uiActionBar.Text("_____________________________________________", GRAY);
    uiActionBar.Newline();
    uiActionBar.Text("Element Inspector:", LIGHTGRAY);
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
        uiActionBar.Text("Select some elements to see their status.");
    }
    uiActionBar.Newline();
    uiActionBar.Text("_____________________________________________", GRAY);
    uiActionBar.Newline();
#endif
}