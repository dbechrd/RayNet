#pragma once
#include "common.h"
#include <cstdio>

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
    int x, y;  // position in spritesheet
    bool collide;
};

struct Tilemap {
    struct Coord {
        int x, y;
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

    void Free(void);
    ~Tilemap();

    Err HexifyFile(const char *filename);
    Err MakeBackup(const char *filename);
    Err Save(const char *filename);
    Err Load(const char *filename);

    // Tiles
    Tile At(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, Tile &tile);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);
    void Set(uint32_t x, uint32_t y, Tile tile);
    void ResolveEntityTerrainCollisions(Entity &entity);
    void Draw(Camera2D &camera);

    // Paths
    AiPath *GetPath(uint32_t pathId) {
        assert(pathId < pathCount);
        return &paths[pathId];
    }
    uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
        assert(pathId < pathCount);
        AiPath *path = &paths[pathId];
        assert(pathNodeIndex < path->pathNodeIndexCount);
        uint32_t nextPathNodeIndex = (pathNodeIndex + 1) % path->pathNodeIndexCount;
        return nextPathNodeIndex;
    }
    AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
        assert(pathId < pathCount);
        AiPath *path = &paths[pathId];
        assert(pathNodeIndex < path->pathNodeIndexCount);
        uint32_t pathNodeId = pathNodeIndices[path->pathNodeIndexOffset + pathNodeIndex];
        return &pathNodes[pathNodeId];
    }

};