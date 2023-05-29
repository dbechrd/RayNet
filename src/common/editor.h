#pragma once
#include "common.h"
#include "tilemap.h"
#include "ui/ui.h"
#include "../common/wang.h"

enum EditMode {
    EditMode_Tiles,
    EditMode_Wang,
    EditMode_Paths,
    EditMode_Warps,
    EditMode_Entities,
    EditMode_Count
};

struct EditModeTiles {
    bool editCollision;

    struct {
        Tile tileDefId;
    } cursor;
};

struct EditModeWang {
    WangTileset wangTileset;
    WangMap wangMap;
};

struct EditModeEntities {
    int testId = 0;
};

struct EditModePathNodes {
    struct {
        bool dragging;
        Vector2 dragStartPosition;

        // NOTE: Make this a tagged union if we want to drag other stuff too
        uint32_t dragPathId;
        uint32_t dragPathNodeIndex;
    } cursor;
};

struct EditModeState {
    bool showColliders{};
    bool showTileIds{};
    bool showEntityIds{};
    EditModeTiles tiles;
    EditModeWang wang;
    EditModeEntities entities;
    EditModePathNodes pathNodes;
};

struct Editor {
    bool active{};
    EditMode mode{};
    EditModeState state{};

    Err Init(void);
    void HandleInput(Camera2D &camera);
    void DrawGroundOverlays(Tilemap &map, Camera2D &camera, double now);
    void DrawEntityOverlays(Tilemap &map, Camera2D &camera, double now);
    UIState DrawUI(Vector2 position, Tilemap &map, double now);

private:
    // Ground overlays (above tiles, below entities)
    void DrawGroundOverlay_Tiles(Tilemap &map, Camera2D &camera, double now);
    void DrawGroundOverlay_Wang(Tilemap &map, Camera2D &camera, double now);
    void DrawGroundOverlay_Paths(Tilemap &map, Camera2D &camera, double now);
    void DrawGroundOverlay_Warps(Tilemap &map, Camera2D &camera, double now);

    // Entity overlays (above entities)
    void DrawEntityOverlay_Collision(Tilemap &map, Camera2D &camera, double now);

    // Action bar and mode tabs
    UIState DrawUI_ActionBar(Vector2 position, Tilemap &map, double now);
    void DrawUI_TileActions(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_Tilesheet(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_Wang(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_EntityActions(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_PathActions(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_WarpActions(UI &uiActionBar, Tilemap &map, double now);
};