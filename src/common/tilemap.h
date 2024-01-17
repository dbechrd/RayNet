#pragma once
#include "common.h"

enum TileLayerType {
    TILE_LAYER_GROUND,
    TILE_LAYER_OBJECT,
    TILE_LAYER_COUNT
};

struct TileChunk {
    uint16_t layers[TILE_LAYER_COUNT][SV_MAX_TILE_CHUNK_WIDTH * SV_MAX_TILE_CHUNK_WIDTH]{};
};

struct Tilemap {
    struct Coord {
        int x, y;

        bool operator==(const Coord &other) const
        {
            return x == other.x && y == other.y;
        }

        struct Hasher {
            size_t operator()(const Coord &coord) const
            {
                return hash_combine(coord.x, coord.y);
            }
        };
    };
    typedef std::unordered_set<Coord, Coord::Hasher> CoordSet;

    struct Region {
        Coord tl;
        Coord br;
    };

    static const DataType dtype = DAT_TYP_TILE_MAP;
    static const uint32_t MAGIC = 0xDBBB9192;
    static const uint16_t VERSION = 9;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    // v5: added warps
    // v6: added auto-tile mask to tileDef
    // v7: tileDefCount no longer based on texture size, in case texture is moved/deleted
    // v8: add sentinel
    // v9: Vector3 path nodes

    typedef std::array<std::vector<uint16_t>, TILE_LAYER_COUNT> TileLayers;

#define HQT_TILE_MAP_FIELDS(FIELD, userdata) \
    FIELD(uint16_t               , version     , {}, HAQ_SERIALIZE           , true, userdata) /* version on disk                */ \
    FIELD(uint16_t               , id          , {}, HAQ_SERIALIZE           , true, userdata) /* id of the map (for networking) */ \
    FIELD(std::string            , name        , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) /* name of map area               */ \
    FIELD(uint16_t               , width       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) /* width of map in tiles          */ \
    FIELD(uint16_t               , height      , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) /* height of map in tiles         */ \
    FIELD(std::string            , title       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) /* display name                   */ \
    FIELD(std::string            , bg_music    , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) /* background music               */ \
    FIELD(TileLayers             , layers      , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::vector<ObjectData>, object_data , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::vector<AiPathNode>, path_nodes  , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::vector<AiPath>    , paths       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_TILE_MAP_FIELDS(HAQ_C_FIELD, 0);

    //-------------------------------
    // Not serialized
    //-------------------------------
    //uint16_t net_id             {};  // for communicating efficiently w/ client about which map
    double      chunkLastUpdatedAt {};  // used by server to know when chunks are dirty on clients
    CoordSet    dirtyTiles         {};  // tiles that have changed since last snapshot was sent
    Edge::Array edges              {};  // collision edge list

    //-------------------------------
    // Clean this section up
    //-------------------------------
    // Tiles
    uint16_t At(TileLayerType layer, uint16_t x, uint16_t y);
    bool AtTry(TileLayerType layer, int x, int y, uint16_t &tile_id);
    bool WorldToTileIndex(int world_x, int world_y, Coord &coord);
    bool AtWorld(TileLayerType layer, int world_x, int world_y, uint16_t &tile_id);
    bool IsSolid(int x, int y);  // tile x,y coord, returns true if out of bounds

    void Autotile(TileLayerType layer, uint16_t x, uint16_t y, double now);
    void Set(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now, bool autotile = true);
    bool SetTry(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now, bool autotile = true);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(TileLayerType layer, uint16_t x, uint16_t y, uint16_t new_tile_id, double now);

    // Objects
    ObjectData *GetObjectData(uint16_t x, uint16_t y);

    AiPath *GetPath(uint16_t pathId);
    uint16_t GetNextPathNodeIndex(uint16_t pathId, uint16_t pathNodeIndex);
    AiPathNode *GetPathNode(uint16_t pathId, uint16_t pathNodeIndex);

    void Update(double now, bool simulate);

    void ResolveEntityCollisionsEdges(Entity &entity);
    void ResolveEntityCollisionsTriggers(Entity &entity);

    void Draw(Camera2D &camera, DrawCmdQueue &sortedDraws);
    void DrawColliders(Camera2D &camera);
    void DrawEdges(void);
    void DrawTileIds(Camera2D &camera);

private:
    void UpdatePower(double now);
    void UpdateEdges(Edge::Array &edges);

    bool NeedsFill(TileLayerType layer, uint16_t x, uint16_t y, uint16_t old_tile_id);
    void Scan(TileLayerType layer, uint16_t lx, uint16_t rx, uint16_t y, uint16_t old_tile_id, std::stack<Coord> &stack);
};