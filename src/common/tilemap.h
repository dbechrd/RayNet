#pragma once
#include "common.h"
#include "net/net.h"
#include "texture_catalog.h"

#define AI_PATH_NONE UINT32_MAX

struct WangMap;

typedef uint8_t Tile;

struct AiPathNode {
    Vector2 pos;
    double waitFor;
};

struct AiPath {
    uint32_t pathNodeIndexOffset;
    uint32_t pathNodeIndexCount;
};

struct TileDef {
    uint32_t x, y;  // position in spritesheet
    bool collide;
    Color color;  // color for minimap/wang tile editor (top left pixel of tile)
};

struct Tilemap {
    struct Coord {
        uint32_t x, y;
    };

    const char *filename;
    const uint32_t MAGIC = 0xDBBB9192;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    const uint32_t VERSION = 4;
    TextureId textureId;  // generated upon load, used to look up in rnTextureCatalog
    char *texturePath;
    uint32_t tileDefCount;
    uint32_t width;  // width of map in tiles
    uint32_t height;  // height of map in tiles
    uint32_t pathNodeCount;
    uint32_t pathNodeIndexCount;
    uint32_t pathCount;
    TileDef *tileDefs;
    Tile *tiles;
    AiPathNode *pathNodes; // 94 19 56 22 57
    uint32_t *pathNodeIndices;  // 0 1 2 | 3 4 5
    AiPath *paths;  // offset, length | 0, 3 | 3, 3

    // TODO: Actually have more than 1 chunk..
    double chunkLastUpdatedAt;  // used by server to know when chunks are dirty on clients

    void SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y);
    void CL_DeserializeChunk(Msg_S_TileChunk &tileChunk);

    ~Tilemap();

    Err Save(const char *filename);
    Err Load(const char *filename, double now);
    static Err ChangeTileset(Tilemap &map, const char *newTexturePath, double now);
    void Unload(void);

    // Tiles
    Tile At(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, Tile &tile);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);

    void Set(uint32_t x, uint32_t y, Tile tile, double now);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(uint32_t x, uint32_t y, int tileDefId, double now);
    void ResolveEntityTerrainCollisions(Entity &entity);

    Rectangle TileDefRect(Tile tile);
    Color TileDefAvgColor(Tile tile);
    void DrawTile(Tile tile, Vector2 position);
    void Draw(Camera2D &camera);
    void DrawColliders(Camera2D &camera);

    // Paths
    AiPath *GetPath(uint32_t pathId) {
        if (pathId < pathCount) {
            return &paths[pathId];
        }
        return 0;
    }
    uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
        AiPath *path = GetPath(pathId);
        if (path && pathNodeIndex < path->pathNodeIndexCount) {
            uint32_t nextPathNodeIndex = (pathNodeIndex + 1) % path->pathNodeIndexCount;
            return nextPathNodeIndex;
        }
        return 0;
    }
    AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
        AiPath *path = GetPath(pathId);
        if (path && pathNodeIndex < path->pathNodeIndexCount) {
            uint32_t pathNodeId = pathNodeIndices[path->pathNodeIndexOffset + pathNodeIndex];
            return &pathNodes[pathNodeId];
        }
        return 0;
    }

private:
    bool NeedsFill(uint32_t x, uint32_t y, int tileDefFill);
    void Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Tilemap::Coord> &stack);
};