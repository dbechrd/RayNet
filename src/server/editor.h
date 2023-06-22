#pragma once
#include "../common/common.h"
#include "../common/tilemap.h"
#include "../common/ui/ui.h"
#include "../common/wang.h"

struct GameServer;

enum EditMode {
    EditMode_Maps,
    EditMode_Tiles,
    EditMode_Wang,
    EditMode_Paths,
    EditMode_Warps,
    EditMode_Entities,
    EditMode_Count
};

enum TileEditMode {
    TileEditMode_Select,
    TileEditMode_Collision,
    TileEditMode_AutoTileMask,
    TileEditMode_Count
};

struct EditModeTiles {
    TileEditMode tileEditMode;

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
    Tilemap *map{};

    Editor(Tilemap *map) : map(map) {}
    Err Init(void);
    void HandleInput(Camera2D &camera);
    void DrawGroundOverlays(Camera2D &camera, double now);
    void DrawEntityOverlays(Camera2D &camera, double now);
    UIState DrawUI(Vector2 position, GameServer &server, double now);

private:
    // Ground overlays (above tiles, below entities)
    void DrawGroundOverlay_Tiles(Camera2D &camera, double now);
    void DrawGroundOverlay_Wang(Camera2D &camera, double now);
    void DrawGroundOverlay_Paths(Camera2D &camera, double now);
    void DrawGroundOverlay_Warps(Camera2D &camera, double now);

    // Entity overlays (above entities)
    void DrawEntityOverlay_Collision(Camera2D &camera, double now);

    // Action bar and mode tabs
    UIState DrawUI_ActionBar(Vector2 position, GameServer &server, double now);
    void DrawUI_MapActions(UI &uiActionBar, GameServer &server, double now);
    void DrawUI_TileActions(UI &uiActionBar, double now);
    void DrawUI_Tilesheet(UI &uiActionBar, double now);
    void DrawUI_Wang(UI &uiActionBar, double now);
    void DrawUI_EntityActions(UI &uiActionBar, double now);
    void DrawUI_PathActions(UI &uiActionBar, double now);
    void DrawUI_WarpActions(UI &uiActionBar, double now);
};