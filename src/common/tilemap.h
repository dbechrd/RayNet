#pragma once
#include "common.h"
#include "data.h"
#include "net/net.h"

#define AI_PATH_NONE UINT32_MAX

struct WangMap;

 struct TileRef {
     uint32_t mapIndex;
     uint32_t x;
     uint32_t y;

     TileRef(uint32_t mapIndex, uint32_t x, uint32_t y)
         : mapIndex(mapIndex), x(x), y(y) {};
 };

struct Tilemap : data::TileMapData {
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
    // v8: add sentinel
    static const uint32_t VERSION = 8;
    static const uint32_t SENTINEL = 0x12345678;

    void SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y);
    void CL_DeserializeChunk(Msg_S_TileChunk &tileChunk);

    Err Save(std::string path);
    Err Load(std::string path);

    // Tiles
    Tile At(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, Tile &tile);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);

    void Set(uint32_t x, uint32_t y, Tile tile, double now);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(uint32_t x, uint32_t y, int tileDefId, double now);

    const data::TileDef &GetTileDef(Tile tile);
    Rectangle TileDefRect(Tile tile);
    Color TileDefAvgColor(Tile tile);

    data::AiPath *GetPath(uint32_t pathId);
    uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex);
    data::AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex);

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