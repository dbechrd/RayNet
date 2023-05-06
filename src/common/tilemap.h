#pragma once
#include "common.h"

#define AI_PATH_NONE UINT32_MAX

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
};

struct Tilemap {
    struct Coord {
        uint32_t x, y;
    };

    const uint32_t MAGIC = 0xDBBB9192;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    const uint32_t VERSION = 3;
    uint32_t texturePathLen;
    char *texturePath;
    Texture2D texture;  // not in file, loaded from texturePath
    uint32_t tileDefCount;
    uint32_t width;
    uint32_t height;
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

    void Free(void);
    ~Tilemap();

    Err Save(const char *filename);
    Err Load(const char *filename);

    // Tiles
    Tile At(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, Tile &tile);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);
    void Set(uint32_t x, uint32_t y, Tile tile, double now);
    void ResolveEntityTerrainCollisions(Entity &entity);
    void Draw(Camera2D &camera, bool showCollision);

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
};