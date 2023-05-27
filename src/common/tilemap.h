#pragma once
#include "common.h"
#include "data.h"
#include "entity.h"
#include "net/net.h"
#include "texture_catalog.h"

#define AI_PATH_NONE UINT32_MAX

struct WangMap;

struct AiPathNode {
    Vector2 pos;
    double waitFor;
};

struct AiPath {
    uint32_t pathNodeIndexOffset;
    uint32_t pathNodeIndexCount;
};

typedef int TileMatId;
struct TileMat {
    StringId sndFootstep;
};

struct TileDef {
    uint32_t x, y;  // position in spritesheet
    bool collide;
    Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    TileMatId materialId;
};

 struct Warp {
     Rectangle collider{};
     Vector2 destPos{};

     // You either need this
     std::string destMap{};          // regular map to warp to
     // Or both of these
     std::string templateMap{};      // template map to make a copy of for procgen
     std::string templateTileset{};  // wang tileset to use for procgen
 };

typedef uint8_t Tile;
struct Tilemap {
    struct Coord {
        uint32_t x, y;
    };

    const uint32_t MAGIC = 0xDBBB9192;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    // v5: added warps
    const uint32_t VERSION = 5;

    std::string filename{};
    StringId textureId{};  // generated upon load, used to look up in rnTextureCatalog

    uint32_t width{};  // width of map in tiles
    uint32_t height{};  // height of map in tiles

    // TODO(dlb): Move these to a global pool, each has its own textureId
    std::vector<TileDef> tileDefs{};
    std::vector<uint8_t> tiles{};
    std::vector<AiPathNode> pathNodes{}; // 94 19 56 22 57
    std::vector<uint32_t> pathNodeIndices{};  // 0 1 2 | 3 4 5
    std::vector<AiPath> paths{};  // offset, length | 0, 3 | 3, 3
    std::vector<Warp> warps{};

    // TODO: Actually have more than 1 chunk..
    double chunkLastUpdatedAt{};  // used by server to know when chunks are dirty on clients

    // [0]: reserved for safe null
    // [1, SV_MAX_PLAYERS]: reserved for player entities
    // [SV_MAX_PLAYERS + 1, SV_MAX_ENTITIES - 1]: dynamic entities (uses freelist)
    uint32_t entity_freelist{};

    // TODO: Rename these so they don't collide with local variables all the time
    Entity          entities  [SV_MAX_ENTITIES]{};
    AspectCollision collision [SV_MAX_ENTITIES]{};
    AspectDialog    dialog    [SV_MAX_ENTITIES]{};
    AspectLife      life      [SV_MAX_ENTITIES]{};
    AspectPathfind  pathfind  [SV_MAX_ENTITIES]{};
    AspectPhysics   physics   [SV_MAX_ENTITIES]{};
    data::Sprite    sprite    [SV_MAX_ENTITIES]{};

    void SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y);
    void CL_DeserializeChunk(Msg_S_TileChunk &tileChunk);

    ~Tilemap();

    Err Save(std::string path);
    Err Load(std::string path, double now);
    static Err ChangeTileset(Tilemap &map, StringId newTextureId, double now);
    void Unload(void);

    // Tiles
    Tile At(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, Tile &tile);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);

    void Set(uint32_t x, uint32_t y, Tile tile, double now);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(uint32_t x, uint32_t y, int tileDefId, double now);

    const TileDef &GetTileDef(Tile tile);
    Rectangle TileDefRect(Tile tile);
    Color TileDefAvgColor(Tile tile);

    AiPath *GetPath(uint32_t pathId);
    uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex);
    AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex);

    uint32_t CreateEntity(EntityType entityType);
    bool SpawnEntity(uint32_t entityId, double now);
    Entity *GetEntity(uint32_t entityId);
    bool DespawnEntity(uint32_t entityId, double now);
    void DestroyEntity(uint32_t entityId);

    Rectangle EntityRect(uint32_t entityId);
    Vector2 EntityTopCenter(uint32_t entityId);
    void EntityTick(uint32_t entityId, double dt);
    void ResolveEntityTerrainCollisions(uint32_t entityId);
    void ResolveEntityWarpCollisions(uint32_t entityId, double now);

    void DrawTile(Tile tile, Vector2 position);
    void Draw(Camera2D &camera);
    void DrawColliders(Camera2D &camera);
    void DrawTileIds(Camera2D &camera);
    void DrawEntityHoverInfo(uint32_t entityId);
    void DrawEntity(uint32_t entityId, double now);

private:
    bool NeedsFill(uint32_t x, uint32_t y, int tileDefFill);
    void Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Tilemap::Coord> &stack);
};