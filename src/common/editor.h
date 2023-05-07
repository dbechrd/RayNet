#pragma once
#include "common.h"

struct Tilemap;
struct UI;
struct UIState;

enum EditMode {
    EditMode_Tiles,
    EditMode_Paths,
    EditMode_Count
};

struct EditModeTiles {
    bool editCollision;

    struct {
        int tileDefId;
    } cursor;
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
    EditModeTiles tiles;
    EditModePathNodes pathNodes;
};

struct Editor {
    bool active{};

    EditMode mode{};
    EditModeState state{};

    void DrawOverlays(IO &io, Tilemap &map, Camera2D &camera, double now);
    UIState DrawUI(IO &io, Vector2 position, Tilemap &map);

private:
    void HandleInput(IO &io, Camera2D &camera);
    void DrawOverlay_Tiles(IO &io, Tilemap &map, Camera2D &camera, double now);
    void DrawOverlay_Paths(IO &io, Tilemap &map, Camera2D &camera);

    UIState DrawUI_ActionBar(IO &io, Vector2 position, Tilemap &map);
    void DrawUI_TileActions(IO &io, UI &uiActionBar, Tilemap &map);
    void DrawUI_PathActions(IO &io, UI &uiActionBar);
};