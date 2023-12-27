#pragma once
#include "../common/common.h"
#include "../common/data.h"
#include "../common/ui/ui.h"
#include "../common/wang.h"

struct GameServer;

enum EditMode {
    EditMode_Maps,
    EditMode_Tiles,
    EditMode_Wang,
    EditMode_Paths,
    EditMode_Dialog,
    EditMode_Entities,
    EditMode_PackFiles,
    EditMode_Debug,
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
    TileLayerType layer;
    struct {
        uint16_t tile_id[TILE_LAYER_COUNT];
    } cursor;
};

struct EditModeWang {
    WangTileset wangTileset;
    WangMap wangMap;
    int hTex = -1;
    int vTex = -1;
};

struct EditModePathNodes {
    struct {
        bool dragging;
        Vector2 dragStartPosition;

        // NOTE: Make this a tagged union if we want to drag other stuff too
        uint16_t dragPathId;
        uint16_t dragPathNodeIndex;
    } cursor;
};

struct EditModeDialog {
    uint16_t dialogId{};
};

struct EditModeEntities {
    uint32_t selectedId{};
};

struct EditModeSfxFiles {
    std::string selectedSfx{};  // yuck :(
};

struct EditModePackFiles {
    Pack *selectedPack{};
    int selectedPackEntryOffset{};
};

struct EditModeDebug {
    int foo{};
};

struct EditModeState {
    bool showColliders {};
    bool showTileEdges {};
    bool showTileIds   {};
    bool showEntityIds {};

    EditModeTiles     tiles     {};
    EditModeWang      wang      {};
    EditModeDialog    dialog    {};
    EditModeEntities  entities  {};
    EditModePathNodes pathNodes {};
    EditModeSfxFiles  sfxFiles  {};
    EditModePackFiles packFiles {};
    EditModeDebug     debug     {};
};

struct Editor {
    bool          active   {};
    EditMode      mode     {};
    EditModeState state    {};
    uint16_t      map_id   {};

    Editor(uint16_t map_id) : map_id(map_id) {}
    Err Init(void);
    void HandleInput(Camera2D &camera);
    void DrawGroundOverlays(Camera2D &camera, double now);
    void DrawEntityOverlays(Camera2D &camera, double now);
    UIState DrawUI(Vector2 position, double now);

private:
    // Ground overlays (above tiles, below entities)
    void DrawGroundOverlay_Tiles(Camera2D &camera, double now);
    void DrawGroundOverlay_Wang(Camera2D &camera, double now);
    void DrawGroundOverlay_Paths(Camera2D &camera, double now);

    // Entity overlays (above entities)
    void DrawEntityOverlay_Collision(Camera2D &camera, double now);

    // Action bar and mode tabs
    UIState DrawUI_ActionBar(Vector2 position, double now);
    void DrawUI_MapActions(UI &uiActionBar, double now);
    void DrawUI_TileActions(UI &uiActionBar, double now);
    void DrawUI_Tilesheet(UI &uiActionBar, double now);
    void DrawUI_WangTile(double now);
    void DrawUI_Wang(UI &uiActionBar, double now);
    void DrawUI_PathActions(UI &uiActionBar, double now);
    void DrawUI_DialogActions(UI &uiActionBar, double now);
    void DrawUI_EntityActions(UI &uiActionBar, double now);
    void DrawUI_PackFiles(UI &uiActionBar, double now);
    void DrawUI_Debug(UI &uiActionBar, double now);
};