#pragma once
#include "../common/common.h"
#include "../common/data.h"
#include "../common/ui/ui.h"
#include "../common/wang.h"

struct GameServer;

enum EditMode {
    EditMode_Maps,
    EditMode_Tiles,
    EditMode_Objects,
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

enum SelectionType {
    SELECTION_MAP,     // Tiles picked from map
    SELECTION_PALETTE, // Tiles picked from tileset palette
    SELECTION_COUNT
};

struct EditModeTiles {
    TileEditMode tileEditMode;
    TileLayerType layer;
    struct {
        // Tile picking from the tile palette
        Tilemap::Coord pick_start{};
        Tilemap::Coord pick_current{};
        Tilemap::Region PickRegion(void) {
            Tilemap::Region region{
                {
                    MIN(pick_start.x, pick_current.x),
                    MIN(pick_start.y, pick_current.y)
                },
                {
                    MAX(pick_start.x, pick_current.x),
                    MAX(pick_start.y, pick_current.y)
                }
            };
            return region;
        }

        Tilemap::Coord SelectionCenter(void) {
            Tilemap::Coord center{};
            center.x = selection_size.x / 2;
            center.y = selection_size.y / 2;
            return center;
        }

        SelectionType selection_type{};
        Tilemap::Coord selection_size{};
        std::vector<uint16_t> selection_tiles[TILE_LAYER_COUNT];
    } cursor;
};


struct EditModeObjects {
    Tilemap::Coord selected_coord;
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

struct EditorSelections {
    uint16_t byType [DAT_TYP_COUNT]{};
};

struct GfxAnimEditor {
    int foo{};
};

struct EditModeState {
    bool showColliders {};
    bool showTileEdges {};
    bool showTileIds   {};
    bool showEntityIds {};

    EditModeTiles     tiles     {};
    EditModeObjects   objects   {};
    EditModeWang      wang      {};
    EditModeDialog    dialog    {};
    EditModeEntities  entities  {};
    EditModePathNodes pathNodes {};
    EditModeSfxFiles  sfxFiles  {};
    EditModePackFiles packFiles {};
    EditModeDebug     debug     {};

    EditorSelections  selections {};
    GfxAnimEditor     gfxAnimEditor {};
};

struct Editor {
    bool          active     {};
    float         width      { 430.0f };
    bool          dock_left  { true };
    EditMode      mode       {};
    EditModeState state      {};
    uint16_t      map_id     {};

    double now;
    double dt;

    // TODO: Make this an enum! Doh.
    bool showGfxFrameEditor      { 1 };
    bool showGfxFrameEditorDirty { 0 };
    bool showGfxAnimEditor       { 0 };
    bool showGfxAnimEditorDirty  { 0 };
    bool showSpriteEditor        { 0 };
    bool showSpriteEditorDirty   { 0 };
    bool showTileDefEditor       { 0 };
    bool showTileDefEditorDirty  { 0 };

    inline void ResetEditorFlags(void) {
        showGfxFrameEditor      = false;
        showGfxFrameEditorDirty = false;
        showGfxAnimEditor       = false;
        showGfxAnimEditorDirty  = false;
        showSpriteEditor        = false;
        showSpriteEditorDirty   = false;
        showTileDefEditor       = false;
        showTileDefEditorDirty  = false;
    }

    Editor(uint16_t map_id) : map_id(map_id) {}
    Err Init(void);
    void HandleInput(Camera2D &camera, double now, double dt);
    void DrawGroundOverlays(Camera2D &camera);
    void DrawEntityOverlays(Camera2D &camera);
    void DrawUI(Camera2D &camera);

private:
    void CenterCameraOnMap(Camera2D &camera);

    // Ground overlays (above tiles, below entities)
    void DrawGroundOverlay_Tiles(Camera2D &camera);
    void DrawGroundOverlay_Objects(Camera2D &camera);
    void DrawGroundOverlay_Wang(Camera2D &camera);
    void DrawGroundOverlay_Paths(Camera2D &camera);

    // Entity overlays (above entities)
    void DrawEntityOverlay_Collision(Camera2D &camera);

    // Action bar and mode tabs
    void DrawUI_ActionBar(void);
    void DrawUI_MapActions(UI &ui);
    void DrawUI_TileActions(UI &ui);
    void DrawUI_ObjectActions(UI &ui);
    void DrawUI_Tilesheet(UI &ui);
    void DrawUI_WangTile(void);
    void DrawUI_Wang(UI &ui);
    void DrawUI_PathActions(UI &ui);
    void DrawUI_DialogActions(UI &ui);
    void DrawUI_EntityActions(UI &ui);
    void DrawUI_PackFiles(UI &ui);
    void DrawUI_Debug(UI &ui);

    void BeginSearchBox(UI &ui, SearchBox &searchBox);

    typedef bool SearchFilterFn(Editor &editor, Pack &pack, const void *dat);
    template <typename T>
    void PackSearchBox(UI &ui, Pack &pack, SearchBox &searchBox, SearchFilterFn *customFilter = 0);

    void DrawUI_GfxFrameEditor(void);
    void DrawUI_GfxAnimEditor(void);
    void DrawUI_SpriteEditor(void);
    void DrawUI_TileDefEditor(void);
};