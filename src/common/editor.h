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

    void DrawOverlays(Tilemap &map, Camera2D &camera, double now);
    UIState DrawUI(Vector2 position, Tilemap &map, double now);

private:
    void HandleInput(Camera2D &camera);
    void DrawOverlay_Tiles(Tilemap &map, Camera2D &camera, double now);
    void DrawOverlay_Paths(Tilemap &map, Camera2D &camera);

    UIState DrawUI_ActionBar(Vector2 position, Tilemap &map, double now);
    void DrawUI_Tilesheet(UI &uiActionBar, Tilemap &map, double now);
    void DrawUI_TileActions(UI &uiActionBar, Tilemap &map);
    void DrawUI_PathActions(UI &uiActionBar);
};