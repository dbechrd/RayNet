#include "collision.h"
#include "file_utils.h"
#include "net/net.h"
#include "texture_catalog.h"
#include "tilemap.h"
#include "wang.h"

void Tilemap::SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y)
{
    tileChunk.map_id = id;
    tileChunk.x = x;
    tileChunk.y = y;
    for (uint32_t ty = y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
        for (uint32_t tx = x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
            AtTry(tx, ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx]);
        }
    }
}

void Tilemap::CL_DeserializeChunk(Msg_S_TileChunk &tileChunk)
{
    if (id = tileChunk.map_id) {
        for (uint32_t ty = tileChunk.y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
            for (uint32_t tx = tileChunk.x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
                Set(tileChunk.x + tx, tileChunk.y + ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx], 0);
            }
        }
    } else {
        printf("[tilemap] Failed to deserialize chunk with mapId %u into map with id %u\n", tileChunk.map_id, id);
    }
}

#if 1
Err Tilemap::Save(std::string path)
{
    Err err = RN_SUCCESS;

    if (FileExists(path.c_str())) {
        err = MakeBackup(path.c_str());
        if (err) return err;
    }

#define WRITE_SENTINEL fwrite(&SENTINEL, sizeof(SENTINEL), 1, file);

    FILE *file = fopen(path.c_str(), "wb");
    do {
        if (!file) {
            err = RN_BAD_FILE_WRITE; break;
        }
        // this isn't technically necessary, but it seems really bizarre to save
        // a tilemap that points to the placeholder texture. the user should
        // really pick a valid tileset image before saving the map
        if (!texture.size()) {
            err = RN_INVALID_PATH; break;
        }
        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }


        fwrite(&MAGIC, sizeof(MAGIC), 1, file);
        fwrite(&VERSION, sizeof(VERSION), 1, file);
        WRITE_SENTINEL

        //// NOTE(dlb): We need to store this explicitly in case the texture file
        //// goes missing, so that we can calculate tileDefs.size() and read the
        //// rest of the file successfully.
        //const TextureCatalog::Entry &texEntry = rnTextureCatalog.GetEntry(textureId);
        //fwrite(&texEntry.image.width, sizeof(texEntry.image.width), 1, file);
        //fwrite(&texEntry.image.height, sizeof(texEntry.image.height), 1, file);

        const std::string texturePath = texture; //rnStringCatalog.GetString(textureId.c_str());
        const int texturePathLen = texturePath.size();
        fwrite(&texturePathLen, sizeof(texturePathLen), 1, file);
        fwrite(texturePath.c_str(), 1, texturePathLen, file);
        WRITE_SENTINEL

        fwrite(&width, sizeof(width), 1, file);
        fwrite(&height, sizeof(height), 1, file);
        WRITE_SENTINEL

        uint32_t tileDefCount = tileDefs.size();
        uint32_t pathNodeCount = pathNodes.size();
        uint32_t pathNodeIndexCount = pathNodeIndices.size();
        uint32_t pathCount = paths.size();
        fwrite(&tileDefCount, sizeof(tileDefCount), 1, file);
        fwrite(&pathNodeCount, sizeof(pathNodeCount), 1, file);
        fwrite(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
        fwrite(&pathCount, sizeof(pathCount), 1, file);
        WRITE_SENTINEL

        for (const data::TileDef &tileDef : tileDefs) {
            fwrite(&tileDef.collide, sizeof(tileDef.collide), 1, file);
            fwrite(&tileDef.auto_tile_mask, sizeof(tileDef.auto_tile_mask), 1, file);
        }
        WRITE_SENTINEL

        if (tiles.size() != (size_t)width * height) {
            err = RN_INVALID_SIZE; break;
        }

        for (Tile &tile : tiles) {
            fwrite(&tile, sizeof(tile), 1, file);
        }
        WRITE_SENTINEL

        for (const data::AiPathNode &aiPathNode : pathNodes) {
            float x = aiPathNode.pos.x * 2;
            float y = aiPathNode.pos.y * 2;
            fwrite(&x, sizeof(aiPathNode.pos.x), 1, file);
            fwrite(&y, sizeof(aiPathNode.pos.y), 1, file);
            fwrite(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
        }
        WRITE_SENTINEL

        for (const uint32_t &aiPathNodeIndex : pathNodeIndices) {
            fwrite(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
        }
        WRITE_SENTINEL

        for (const data::AiPath &aiPath : paths) {
            fwrite(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
            fwrite(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
        }
        WRITE_SENTINEL
    } while (0);

#undef WRITE_SENTINEL

    if (file) fclose(file);
    if (!err) {
        name = path;
    }
    return RN_SUCCESS;
}

// TODO(dlb): This shouldn't load in-place.. it causes the game to crash when
// the load fails. Instead, return a new a tilemap and switch over to it after
// it has been loaded successfully.
Err Tilemap::Load(std::string path)
{
    Err err = RN_SUCCESS;

#define READ_SENTINEL\
    if (version >= 8) { \
        fread(&sentinel, sizeof(sentinel), 1, file); \
        if (sentinel != SENTINEL) { \
            assert(!"map file is corrupt"); \
            err = RN_BAD_FILE_READ; break; \
        } \
    }

    FILE *file = fopen(path.c_str(), "rb");
    do {
        if (!file) {
            err = RN_BAD_FILE_READ; break;
        }

        uint32_t magic = 0;
        fread(&magic, sizeof(magic), 1, file);
        if (magic != MAGIC) {
            err = RN_BAD_MAGIC; break;
        }
        fread(&version, sizeof(version), 1, file);

        uint32_t sentinel = 0;
        READ_SENTINEL

        int texturePathLen = 0;
        fread(&texturePathLen, sizeof(texturePathLen), 1, file);
        if (!texturePathLen || texturePathLen > PATH_LEN_MAX) {
            err = RN_INVALID_PATH; break;
        }
        char texturePathBuf[PATH_LEN_MAX + 1]{};
        fread(texturePathBuf, 1, texturePathLen, file);
        READ_SENTINEL

        texture = "gfx_til_overworld"; //texturePathBuf;
        //StringId textureId = rnStringCatalog.AddString(texture);
        //err = rnTextureCatalog.Load(textureId);
        //if (err) {
        //    if (version < 7) {
        //        const char *filename = GetFileName(texture.c_str());
        //        texture = "resources/texture/" + std::string(filename);
        //        textureId = rnStringCatalog.AddString(texture);
        //        err = rnTextureCatalog.Load(textureId);
        //        if (err) {
        //            break;
        //        }
        //    }
        //}
        //
        //const TextureCatalog::Entry &texEntry = rnTextureCatalog.GetEntry(textureId);
        //if (!texEntry.image.width) {
        //    err = RN_BAD_FILE_READ; break;
        //}
        //if (texEntry.texture.width % TILE_W != 0 || texEntry.texture.height % TILE_W != 0) {
        //    err = RN_INVALID_SIZE; break;
        //}

        //SetTextureWrap(texEntry.texture, TEXTURE_WRAP_CLAMP);

        fread(&width, sizeof(width), 1, file);
        fread(&height, sizeof(height), 1, file);
        READ_SENTINEL

        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }

        uint32_t tileDefCount = 0;
        uint32_t pathNodeCount = 0;
        uint32_t pathNodeIndexCount = 0;
        uint32_t pathCount = 0;
        uint32_t warpCount = 0;
        if (version >= 7) {
            fread(&tileDefCount, sizeof(tileDefCount), 1, file);
        } else {
            // TODO(dlb): This is broken.. if texture is missing, then we can't load it
            // which means we don't know the width/height and also cannot read the
            // rest of this file, which requires having an accurate tileDefCount
            // for the fread()s.
            tileDefCount = 25; //((size_t)texEntry.texture.width / TILE_W) * (texEntry.texture.height / TILE_W);

        }
        fread(&pathNodeCount, sizeof(pathNodeCount), 1, file);
        fread(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
        fread(&pathCount, sizeof(pathCount), 1, file);
        if (version < 8) {
            fread(&warpCount, sizeof(warpCount), 1, file);
        }
        READ_SENTINEL

        tileDefs.resize(tileDefCount);
        tiles.resize((size_t)width * height);
        pathNodes.resize(pathNodeCount);
        pathNodeIndices.resize(pathNodeIndexCount);
        paths.resize(pathCount);

        for (uint32_t i = 0; i < tileDefCount; i++) {
            data::TileDef &tileDef = tileDefs[i];
            int tilesPerRow = 5; //texEntry.texture.width / TILE_W;
            tileDef.x = i % tilesPerRow * TILE_W;
            tileDef.y = i / tilesPerRow * TILE_W;
            fread(&tileDef.collide, sizeof(tileDef.collide), 1, file);

            if (version >= 6) {
                fread(&tileDef.auto_tile_mask, sizeof(tileDef.auto_tile_mask), 1, file);
            }

            //assert(tileDef.x < texEntry.image.width);
            //assert(tileDef.y < texEntry.image.height);
            //tileDef.color = GetImageColor(texEntry.image, tileDef.x, tileDef.y);
        }
        READ_SENTINEL

        for (Tile &tile : tiles) {
            fread(&tile, sizeof(tile), 1, file);
            if (tile >= tileDefCount) {
                tile = 0;
                //err = RN_OUT_OF_BOUNDS; break;
            }
        }
        READ_SENTINEL

        for (data::AiPathNode &aiPathNode : pathNodes) {
            fread(&aiPathNode.pos.x, sizeof(aiPathNode.pos.x), 1, file);
            fread(&aiPathNode.pos.y, sizeof(aiPathNode.pos.y), 1, file);
            fread(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
        }
        READ_SENTINEL

        for (uint32_t &aiPathNodeIndex : pathNodeIndices) {
            fread(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
        }
        READ_SENTINEL

        for (data::AiPath &aiPath : paths) {
            fread(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
            fread(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
        }
        READ_SENTINEL
    } while (0);

#undef READ_SENTINEL

    if (file) fclose(file);

    if (!err) {
        name = path;
        printf("[tilemap] Loaded %s successfully\n", name.c_str());

        data::Pack pack = data::Pack("pack/overworld_map.dat");
        pack.tile_maps.push_back(*this);
        Err err2 = data::SavePack(pack);
        assert(!err2);
    }
    return err;
}
#endif

Tile Tilemap::At(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return tiles[(size_t)y * width + x];
}
bool Tilemap::AtTry(uint32_t x, uint32_t y, Tile &tile)
{
    if (x < width && y < height) {
        tile = At(x, y);
        return true;
    }
    return false;
}
bool Tilemap::WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord)
{
    if (world_x < width * TILE_W && world_y < height * TILE_W) {
        coord.x = world_x / TILE_W;
        coord.y = world_y / TILE_W;
        return true;
    }
    return false;
}
bool Tilemap::AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile = At(coord.x, coord.y);
        return true;
    }
    return false;
}

void Tilemap::Set(uint32_t x, uint32_t y, Tile tile, double now)
{
    assert(x < width);
    assert(y < height);
    Tile &mapTile = tiles[(size_t)y * width + x];
    if (mapTile != tile) {
        mapTile = tile;
        chunkLastUpdatedAt = now;
    }
}
void Tilemap::SetFromWangMap(WangMap &wangMap, double now)
{
    // TODO: Specify map coords to set (or chunk id) and do a bounds check here instead
    if (width != wangMap.image.width || height != wangMap.image.height) {
        return;
    }

    uint8_t *pixels = (uint8_t *)wangMap.image.data;
    for (uint32_t y = 0; y < width; y++) {
        for (uint32_t x = 0; x < height; x++) {
            uint8_t tile = pixels[y * width + x];
            tile = tile < (width * height) ? tile : 0;
            Set(x, y, tile, now);
        }
    }
}
bool Tilemap::NeedsFill(uint32_t x, uint32_t y, int tileDefFill)
{
    Tile tile;
    if (AtTry(x, y, tile)) {
        return tile == tileDefFill;
    }
    return false;
}
void Tilemap::Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Coord> &stack)
{
    bool inSpan = false;
    for (uint32_t x = lx; x < rx; x++) {
        if (!NeedsFill(x, y, tileDefFill)) {
            inSpan = false;
        } else if (!inSpan) {
            stack.push({ x, y });
            inSpan = true;
        }
    }
}
void Tilemap::Fill(uint32_t x, uint32_t y, int tileDefId, double now)
{
    Tile tileDefFill = At(x, y);
    if (tileDefFill == tileDefId) {
        return;
    }

    std::stack<Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Tilemap::Coord coord = stack.top();
        stack.pop();

        uint32_t lx = coord.x;
        uint32_t rx = coord.x;
        while (lx && NeedsFill(lx - 1, coord.y, tileDefFill)) {
            Set(lx - 1, coord.y, tileDefId, now);
            lx -= 1;
        }
        while (NeedsFill(rx, coord.y, tileDefFill)) {
            Set(rx, coord.y, tileDefId, now);
            rx += 1;
        }
        if (coord.y) {
            Scan(lx, rx, coord.y - 1, tileDefFill, stack);
        }
        Scan(lx, rx, coord.y + 1, tileDefFill, stack);
    }
}

const data::TileDef &Tilemap::GetTileDef(Tile tile)
{
    if (tile >= tileDefs.size()) tile = 0;
    return tileDefs[tile];
}
Rectangle Tilemap::TileDefRect(Tile tile)
{
    const data::TileDef &tileDef = GetTileDef(tile);
    const Rectangle rect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
    return rect;
}
Color Tilemap::TileDefAvgColor(Tile tile)
{
    const data::TileDef &tileDef = GetTileDef(tile);
    return tileDef.color;
}

data::AiPath *Tilemap::GetPath(uint32_t pathId) {
    if (pathId < paths.size()) {
        return &paths[pathId];
    }
    return 0;
}
uint32_t Tilemap::GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
    data::AiPath *path = GetPath(pathId);
    if (path && pathNodeIndex < path->pathNodeIndexCount) {
        uint32_t nextPathNodeIndex = (pathNodeIndex + 1) % path->pathNodeIndexCount;
        return nextPathNodeIndex;
    }
    return 0;
}
data::AiPathNode *Tilemap::GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
    data::AiPath *path = GetPath(pathId);
    if (path && pathNodeIndex < path->pathNodeIndexCount) {
        uint32_t pathNodeId = pathNodeIndices[(size_t)path->pathNodeIndexOffset + pathNodeIndex];
        return &pathNodes[pathNodeId];
    }
    return 0;
}

void Tilemap::ResolveEntityTerrainCollisions(data::Entity &entity)
{
    entity.colliding = false;

    Vector2 topLeft{
        entity.position.x - entity.radius,
        entity.position.y - entity.radius
    };
    Vector2 bottomRight{
        entity.position.x + entity.radius,
        entity.position.y + entity.radius
    };

    int yMin = CLAMP(floorf(topLeft.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf(bottomRight.y / TILE_W), 0, height);
    int xMin = CLAMP(floorf(topLeft.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf(bottomRight.x / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile{};
            if (AtTry(x, y, tile)) {
                const data::TileDef &tileDef = GetTileDef(tile);
                if (tileDef.collide) {
                    Rectangle tileRect{};
                    Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    tileRect.x = tilePos.x;
                    tileRect.y = tilePos.y;
                    tileRect.width = TILE_W;
                    tileRect.height = TILE_W;
                    Manifold manifold{};
                    if (dlb_CheckCollisionCircleRec(entity.ScreenPos(), entity.radius, tileRect, &manifold)) {
                        entity.colliding = true;
                        Vector2 vel2 = { entity.velocity.x, entity.velocity.y };
                        if (Vector2DotProduct(vel2, manifold.normal) < 0) {
                            entity.position.x += manifold.normal.x * manifold.depth;
                            entity.position.y += manifold.normal.y * manifold.depth;
                        }
                    }
                }
            }
        }
    }
}
void Tilemap::ResolveEntityTerrainCollisions(uint32_t entityId)
{
    assert(entityId);
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity || !entity->Alive() || !entity->radius) {
        return;
    }

    ResolveEntityTerrainCollisions(*entity);
}

void Tilemap::DrawTile(Texture2D tex, Tile tile, Vector2 position)
{
    const Rectangle texRect = TileDefRect(tile);
    //position.x = floorf(position.x);
    //position.y = floorf(position.y);
    if (position.x != floorf(position.x)) assert(!"floating x");
    if (position.y != floorf(position.y)) assert(!"floating y");
    dlb_DrawTextureRec(tex, texRect, position, WHITE);
}
void Tilemap::Draw(Camera2D &camera)
{
    const data::GfxFile &gfx_file = data::packs[0]->FindGraphic(texture);

    Rectangle screenRect = GetScreenRectWorld(camera);
    int yMin = CLAMP(floorf(screenRect.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((screenRect.y + screenRect.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(screenRect.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((screenRect.x + screenRect.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile = At(x, y);
            DrawTile(gfx_file.texture, tile, { (float)x * TILE_W, (float)y * TILE_W });
        }
    }
}
void Tilemap::DrawColliders(Camera2D &camera)
{
    Rectangle screenRect = GetScreenRectWorld(camera);
    int yMin = CLAMP(floorf(screenRect.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((screenRect.y + screenRect.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(screenRect.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((screenRect.x + screenRect.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile = At(x, y);
            const data::TileDef &tileDef = GetTileDef(tile);
            if (tileDef.collide) {
                Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                const int pad = 1;
                DrawRectangleLinesEx(
                    {
                        tilePos.x + pad,
                        tilePos.y + pad,
                        TILE_W - pad * 2,
                        TILE_W - pad * 2,
                    },
                    2.0f,
                    MAROON
                );
            }
        }
    }
}
void Tilemap::DrawTileIds(Camera2D &camera)
{
    Rectangle screenRect = GetScreenRectWorld(camera);
    int yMin = CLAMP(floorf(screenRect.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((screenRect.y + screenRect.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(screenRect.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((screenRect.x + screenRect.width) / TILE_W), 0, width);

    const int pad = 8;
    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile = At(x, y);
            Vector2 tilePos = { (float)x * TILE_W + pad, (float)y * TILE_W + pad };
            //DrawTextEx(fntSmall, TextFormat("%d", tile), tilePos, fntSmall.baseSize * (0.5f / camera.zoom), 1 / camera.zoom, WHITE);
            DrawTextEx(fntSmall, TextFormat("%d", tile), tilePos, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
        }
    }
}
