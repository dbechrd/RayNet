#pragma once
#include "common.h"
#include "data.h"
#include "net/net.h"
#include "strings.h"

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
    uint16_t tileId;
    uint16_t sheetId;
    uint16_t materialId;
    uint16_t x;
    uint16_t y;
    uint8_t w;
    uint8_t h;
    uint8_t collide;
    uint8_t auto_tile_mask;
    Color color;  // color for minimap/wang tile editor (top left pixel of tile)
};

 struct TileRef {
     uint32_t mapIndex;
     uint32_t x;
     uint32_t y;

     TileRef(uint32_t mapIndex, uint32_t x, uint32_t y)
         : mapIndex(mapIndex), x(x), y(y) {};
 };

typedef uint8_t Tile;
struct Tilemap {
    struct Coord {
        uint32_t x, y;
    };

    static const uint32_t MAGIC = 0xDBBB9192;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    // v5: added warps
    // v6: added auto-tile mask to tileDef
    // v7: tileDefCount no longer based on texture size, in case texture is moved/deleted
    static const uint32_t VERSION = 7;

    uint32_t    id        {};
    std::string name      {};  // technically a filename? usually?
    uint32_t    version   {};  // version on disk
    StringId    textureId {};  // generated upon load, used to look up in rnTextureCatalog
    uint32_t    width     {};  // width of map in tiles
    uint32_t    height    {};  // height of map in tiles

    // TODO(dlb): Move these to a global pool, each has its own textureId
    std::vector<TileDef>    tileDefs        {};
    std::vector<uint8_t>    tiles           {};
    std::vector<AiPathNode> pathNodes       {};  // 94 19 56 22 57
    std::vector<uint32_t>   pathNodeIndices {};  // 0 1 2 | 3 4 5
    std::vector<AiPath>     paths           {};  // offset, length | 0, 3 | 3, 3

    // TODO: Actually have more than 1 chunk..
    double chunkLastUpdatedAt{};  // used by server to know when chunks are dirty on clients

    void SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y);
    void CL_DeserializeChunk(Msg_S_TileChunk &tileChunk);

    ~Tilemap();

    Err Save(std::string path);
    Err Load(std::string path);
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

    void ResolveEntityTerrainCollisions(data::Entity &entity);
    void ResolveEntityTerrainCollisions(uint32_t entityId);

    void DrawTile(Texture2D tex, Tile tile, Vector2 position);
    void Draw(Camera2D &camera);
    void DrawColliders(Camera2D &camera);
    void DrawTileIds(Camera2D &camera);

private:
    bool NeedsFill(uint32_t x, uint32_t y, int tileDefFill);
    void Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Tilemap::Coord> &stack);
};