#include "collision.h"
#include "file_utils.h"
#include "texture_catalog.h"
#include "tilemap.h"
#include "wang.h"

void Tilemap::SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y)
{
    tileChunk.mapName = filename;
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
    if (tileChunk.mapName != filename) {
        Load(tileChunk.mapName, 0);
    }
    for (uint32_t ty = tileChunk.y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
        for (uint32_t tx = tileChunk.x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
            Set(tileChunk.x + tx, tileChunk.y + ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx], 0);
        }
    }
}

Tilemap::~Tilemap(void)
{
    Unload();
}

Err Tilemap::Save(std::string path)
{
    Err err = RN_SUCCESS;

    if (FileExists(path.c_str())) {
        err = MakeBackup(path.c_str());
        if (err) return err;
    }

    FILE *file = fopen(path.c_str(), "w");
    do {
        if (!file) {
            err = RN_BAD_FILE_WRITE; break;
        }
        // this isn't technically necessary, but it seems really bizarre to save
        // a tilemap that points to the placeholder texture. the user should
        // really pick a valid tileset image before saving the map
        if (!textureId) {
            err = RN_INVALID_PATH; break;
        }
        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }

        fwrite(&MAGIC, sizeof(MAGIC), 1, file);
        fwrite(&VERSION, sizeof(VERSION), 1, file);

        const std::string texturePath = rnStringCatalog.GetString(textureId);
        const int texturePathLen = texturePath.size();
        fwrite(&texturePathLen, sizeof(texturePathLen), 1, file);
        fwrite(texturePath.c_str(), 1, texturePathLen, file);
        fwrite(&width, sizeof(width), 1, file);
        fwrite(&height, sizeof(height), 1, file);

        uint32_t pathNodeCount = pathNodes.size();
        uint32_t pathNodeIndexCount = pathNodeIndices.size();
        uint32_t pathCount = paths.size();
        uint32_t warpCount = warps.size();
        fwrite(&pathNodeCount, sizeof(pathNodeCount), 1, file);
        fwrite(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
        fwrite(&pathCount, sizeof(pathCount), 1, file);
        fwrite(&warpCount, sizeof(warpCount), 1, file);

        for (const TileDef &tileDef : tileDefs) {
            fwrite(&tileDef.collide, sizeof(tileDef.collide), 1, file);
        }

        if (tiles.size() != (size_t)width * height) {
            err = RN_INVALID_SIZE; break;
        }
        for (Tile &tile : tiles) {
            fwrite(&tile, sizeof(tile), 1, file);
        }

        for (const AiPathNode &aiPathNode : pathNodes) {
            fwrite(&aiPathNode.pos.x, sizeof(aiPathNode.pos.x), 1, file);
            fwrite(&aiPathNode.pos.y, sizeof(aiPathNode.pos.y), 1, file);
            fwrite(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
        }

        for (const uint32_t &aiPathNodeIndex : pathNodeIndices) {
            fwrite(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
        }

        for (const AiPath &aiPath : paths) {
            fwrite(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
            fwrite(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
        }

        for (const Warp &warp : warps) {
            fwrite(&warp.collider.x, sizeof(warp.collider.x), 1, file);
            fwrite(&warp.collider.y, sizeof(warp.collider.y), 1, file);
            fwrite(&warp.collider.width, sizeof(warp.collider.width), 1, file);
            fwrite(&warp.collider.height, sizeof(warp.collider.height), 1, file);

            fwrite(&warp.destPos.x, sizeof(warp.destPos.x), 1, file);
            fwrite(&warp.destPos.y, sizeof(warp.destPos.y), 1, file);

            uint32_t destMapLen = warp.destMap.size();
            fwrite(&destMapLen, 1, sizeof(destMapLen), file);
            fwrite(warp.destMap.c_str(), 1, warp.destMap.size(), file);

            uint32_t templateMapLen = warp.templateMap.size();
            fwrite(&templateMapLen, 1, sizeof(templateMapLen), file);
            fwrite(warp.templateMap.c_str(), 1, warp.templateMap.size(), file);

            uint32_t templateTilesetLen = warp.templateTileset.size();
            fwrite(&templateTilesetLen, 1, sizeof(templateTilesetLen), file);
            fwrite(warp.templateTileset.c_str(), 1, warp.templateTileset.size(), file);
        }
    } while (0);

    if (file) fclose(file);
    if (!err) {
        filename = path;
    }
    return RN_SUCCESS;
}

// TODO(dlb): This shouldn't load in-place.. it causes the game to crash when
// the load fails. Instead, return a new a tilemap and switch over to it after
// it has been loaded successfully.
Err Tilemap::Load(std::string path, double now)
{
    Err err = RN_SUCCESS;

    Unload();

    FILE *file = fopen(path.c_str(), "r");
    do {
        if (!file) {
            err = RN_BAD_FILE_READ; break;
        }

        uint32_t magic = 0;
        fread(&magic, sizeof(magic), 1, file);
        if (magic != MAGIC) {
            err = RN_BAD_MAGIC; break;
        }

        uint32_t version = 0;
        fread(&version, sizeof(version), 1, file);

        int texturePathLen = 0;
        fread(&texturePathLen, sizeof(texturePathLen), 1, file);
        if (!texturePathLen || texturePathLen > PATH_LEN_MAX) {
            err = RN_INVALID_PATH; break;
        }

        char texturePathBuf[PATH_LEN_MAX + 1]{};
        fread(texturePathBuf, 1, texturePathLen, file);

        std::string texturePath{ texturePathBuf };
        textureId = rnStringCatalog.AddString(texturePath);
        rnTextureCatalog.Load(textureId);
        const TextureCatalog::Entry &texEntry = rnTextureCatalog.GetEntry(textureId);
        if (!texEntry.image.width) {
            err = RN_BAD_FILE_READ; break;
        }
        if (texEntry.texture.width % TILE_W != 0 || texEntry.texture.height % TILE_W != 0) {
            err = RN_INVALID_SIZE; break;
        }

        fread(&width, sizeof(width), 1, file);
        fread(&height, sizeof(height), 1, file);
        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }

        uint32_t pathNodeCount = 0;
        uint32_t pathNodeIndexCount = 0;
        uint32_t pathCount = 0;
        uint32_t warpCount = 0;
        fread(&pathNodeCount, sizeof(pathNodeCount), 1, file);
        fread(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
        fread(&pathCount, sizeof(pathCount), 1, file);
        fread(&warpCount, sizeof(warpCount), 1, file);

        // TODO(dlb): This is broken.. if texture is missing, then we can't load it
        // which means we don't know the width/height and also cannot read the
        // rest of this file, which requires having an accurate tileDefCount
        // for the fread()s.
        size_t tileDefCount = ((size_t)texEntry.texture.width / TILE_W) * (texEntry.texture.height / TILE_W);
        tileDefs.resize(tileDefCount);
        tiles.resize((size_t)width * height);
        pathNodes.resize(pathNodeCount);
        pathNodeIndices.resize(pathNodeIndexCount);
        paths.resize(pathCount);
        warps.resize(warpCount);

        for (uint32_t i = 0; i < tileDefCount; i++) {
            TileDef &tileDef = tileDefs[i];
            int tilesPerRow = texEntry.texture.width / TILE_W;
            tileDef.x = i % tilesPerRow * TILE_W;
            tileDef.y = i / tilesPerRow * TILE_W;
            fread(&tileDef.collide, sizeof(tileDef.collide), 1, file);

            assert(tileDef.x < texEntry.image.width);
            assert(tileDef.y < texEntry.image.height);
            tileDef.color = GetImageColor(texEntry.image, tileDef.x, tileDef.y);
        }

        for (Tile &tile : tiles) {
            fread(&tile, sizeof(tile), 1, file);
            if (tile >= tileDefCount) {
                tile = 0;
                //err = RN_OUT_OF_BOUNDS; break;
            }
        }
        chunkLastUpdatedAt = now;

        for (AiPathNode &aiPathNode : pathNodes) {
            fread(&aiPathNode.pos.x, sizeof(aiPathNode.pos.x), 1, file);
            fread(&aiPathNode.pos.y, sizeof(aiPathNode.pos.y), 1, file);
            fread(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
        }

        for (uint32_t &aiPathNodeIndex : pathNodeIndices) {
            fread(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
        }

        for (AiPath &aiPath : paths) {
            fread(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
            fread(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
        }

        for (Warp &warp : warps) {
            if (version >= 5) {
                fread(&warp.collider.x, sizeof(warp.collider.x), 1, file);
                fread(&warp.collider.y, sizeof(warp.collider.y), 1, file);
                fread(&warp.collider.width, sizeof(warp.collider.width), 1, file);
                fread(&warp.collider.height, sizeof(warp.collider.height), 1, file);

                fread(&warp.destPos.x, sizeof(warp.destPos.x), 1, file);
                fread(&warp.destPos.y, sizeof(warp.destPos.y), 1, file);

                uint32_t destMapLen = warp.destMap.size();
                fread(&destMapLen, 1, sizeof(destMapLen), file);
                if (destMapLen > PATH_LEN_MAX) {
                    err = RN_OUT_OF_BOUNDS; break;
                }
                char destMapBuf[PATH_LEN_MAX + 1]{};
                fread(destMapBuf, 1, destMapLen, file);
                warp.destMap = destMapBuf;

                uint32_t templateMapLen = warp.templateMap.size();
                fread(&templateMapLen, 1, sizeof(templateMapLen), file);
                if (templateMapLen > PATH_LEN_MAX) {
                    err = RN_OUT_OF_BOUNDS; break;
                }
                char templateMapBuf[PATH_LEN_MAX + 1]{};
                fread(templateMapBuf, 1, templateMapLen, file);
                warp.templateMap = templateMapBuf;

                uint32_t templateTilesetLen = warp.templateTileset.size();
                fread(&templateTilesetLen, 1, sizeof(templateTilesetLen), file);
                if (templateTilesetLen > PATH_LEN_MAX) {
                    err = RN_OUT_OF_BOUNDS; break;
                }
                char templateTilesetBuf[PATH_LEN_MAX + 1]{};
                fread(templateTilesetBuf, 1, templateTilesetLen, file);
                warp.templateTileset = templateTilesetBuf;
            } else {
                Vector2 TL{ 1632, 404 };
                Vector2 BR{ 1696, 416 };
                Vector2 size = Vector2Subtract(BR, TL);
                Rectangle warpRect{};
                warpRect.x = TL.x;
                warpRect.y = TL.y;
                warpRect.width = size.x;
                warpRect.height = size.y;

                warp.collider = warpRect;
                // Bottom center of warp (assume maps line up and are same size for now)
                warp.destPos.x = BR.x - size.x / 2;
                warp.destPos.y = BR.y;
                warp.templateMap = "maps/cave.dat";
                warp.templateTileset = "resources/wang/tileset2x2.png";
            }
        }
    } while (0);
    if (file) fclose(file);

    if (!err) {
        filename = path;
    } else {
        Unload();
    }
    return err;
}

Err Tilemap::ChangeTileset(Tilemap &map, StringId newTextureId, double now)
{
    Err err = RN_SUCCESS;

    const TextureCatalog::Entry &texEntry = rnTextureCatalog.GetEntry(map.textureId);

    // TODO(dlb): Is this valid?? comparing a std::string to a const char *
    if (newTextureId == map.textureId) {
        assert(!"u wot mate?");
        return RN_SUCCESS;
    }

    Tilemap *newMapPtr = new Tilemap();
    Tilemap &newMap = *newMapPtr;
    do {
        newMap.textureId = newTextureId;
        rnTextureCatalog.Load(newMap.textureId);
        const TextureCatalog::Entry &newTexEntry = rnTextureCatalog.GetEntry(newMap.textureId);
        if (newTexEntry.texture.width % TILE_W != 0 || newTexEntry.texture.height % TILE_W != 0) {
            err = RN_INVALID_SIZE; break;
        }
        if (!newTexEntry.image.width) {
            err = RN_BAD_FILE_READ; break;
        }

        size_t tileDefCount = ((size_t)newTexEntry.texture.width / TILE_W) * (newTexEntry.texture.height / TILE_W);
        newMap.tileDefs.resize(tileDefCount);
        for (int i = 0; i < tileDefCount; i++) {
            TileDef &tileDef = newMap.tileDefs[i];
            int tilesPerRow = newTexEntry.texture.width / TILE_W;
            tileDef.x = i % tilesPerRow * TILE_W;
            tileDef.y = i / tilesPerRow * TILE_W;

            // This probably kinda stupid.. but idk
            if (i < map.tileDefs.size()) {
                TileDef &oldTileDef = map.tileDefs[i];
                tileDef.collide = oldTileDef.collide;
            }

            tileDef.color = GetImageColor(newTexEntry.image, tileDef.x, tileDef.y);
        }

        // TODO(dlb): If we want to be able to change the size of tileset images
        // but still have the old indices somehow resolve to the same art in the
        // new tileset image, we should probably do something fancy like hash
        // the pixel data of the tiles and match them up that way or wutevs.
        //
        // If we did that, maybe it's optional.. because you might want to move
        // tiles around on purpose and have them map to some other tile, right?
        //
        newMap.tiles.resize((size_t)map.width * map.height);
        for (int i = 0; i < map.width * map.height; i++) {
            Tile &src = map.tiles[i];
            Tile &dst = newMap.tiles[i];
            dst = src >= tileDefCount ? 0 : src;
        }
    } while(0);

    if (!err) {
        rnTextureCatalog.Unload(map.textureId);

        map.textureId = newMap.textureId;
        //map.width = newMap.width;
        //map.height = newMap.width;
        map.tileDefs = newMap.tileDefs;
        map.tiles = newMap.tiles;
        //map.pathNodes = newMap.pathNodes;
        //map.pathNodeIndices = newMap.pathNodeIndices;
        //map.paths = newMap.paths;
        map.chunkLastUpdatedAt = now;
    }

    newMap.Unload();
    delete newMapPtr;

    return err;
}

void Tilemap::Unload(void)
{
    rnTextureCatalog.Unload(textureId);

    textureId = STR_NULL;
    tileDefs.clear();
    tiles.clear();
    pathNodes.clear();
    pathNodeIndices.clear();
    paths.clear();

    entity_freelist = SV_FIRST_ENTITY_IDX;  // 0 reserved, 1-MAX_PLAYERS reserved
    for (uint32_t i = entity_freelist; i < SV_MAX_ENTITIES - 1; i++) {
        Entity &entity = entities[i];
        entities[i].freelist_next = i + 1;
    }
}

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
void Tilemap::Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Tilemap::Coord> &stack)
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

const TileDef &Tilemap::GetTileDef(Tile tile)
{
    if (tile >= tileDefs.size()) tile = 0;
    return tileDefs[tile];
}
Rectangle Tilemap::TileDefRect(Tile tile)
{
    const TileDef &tileDef = GetTileDef(tile);
    const Rectangle rect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
    return rect;
}
Color Tilemap::TileDefAvgColor(Tile tile)
{
    const TileDef &tileDef = GetTileDef(tile);
    return tileDef.color;
}

AiPath *Tilemap::GetPath(uint32_t pathId) {
    if (pathId < paths.size()) {
        return &paths[pathId];
    }
    return 0;
}
uint32_t Tilemap::GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path && pathNodeIndex < path->pathNodeIndexCount) {
        uint32_t nextPathNodeIndex = (pathNodeIndex + 1) % path->pathNodeIndexCount;
        return nextPathNodeIndex;
    }
    return 0;
}
AiPathNode *Tilemap::GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path && pathNodeIndex < path->pathNodeIndexCount) {
        uint32_t pathNodeId = pathNodeIndices[(size_t)path->pathNodeIndexOffset + pathNodeIndex];
        return &pathNodes[pathNodeId];
    }
    return 0;
}

uint32_t Tilemap::CreateEntity(EntityType entityType)
{
    uint32_t entityId = entity_freelist;
    if (entity_freelist) {
        Entity &e = entities[entity_freelist];
        entity_freelist = e.freelist_next;
        e.freelist_next = 0;
        e.type = entityType;
    }
    return entityId;
}
bool Tilemap::SpawnEntity(uint32_t entityId, double now)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.spawnedAt = now;
            return true;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return false;
}
Entity *Tilemap::GetEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            return &entity;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return 0;
}
bool Tilemap::DespawnEntity(uint32_t entityId, double now)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.despawnedAt = now;
            return true;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return false;
}
void Tilemap::DestroyEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entities  [entityId] = {};
            physics   [entityId] = {};
            collision [entityId] = {};
            life      [entityId] = {};
            pathfind  [entityId] = {};
            sprite    [entityId] = {};
            if (entityId >= SV_FIRST_ENTITY_IDX) {
                entity.freelist_next = entity_freelist;
                entity_freelist = entityId;
            }
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}

Rectangle Tilemap::EntityRect(uint32_t entityId)
{
    Entity &entity = entities[entityId];
    const data::GfxFrame &frame = data::GetSpriteFrame(sprite[entityId]);
    const Rectangle rect{
        entity.position.x - (float)(frame.w / 2),
        entity.position.y - (float)frame.h,
        (float)frame.w,
        (float)frame.h
    };
    return rect;
}
Vector2 Tilemap::EntityTopCenter(uint32_t entityId)
{
    const Rectangle rect = EntityRect(entityId);
    const Vector2 topCenter{
        rect.x + rect.width / 2,
        rect.y
    };
    return topCenter;
}
void Tilemap::EntityTick(uint32_t entityId, double dt)
{
    Entity &entity = entities[entityId];
    AspectPhysics &physics = this->physics[entityId];

    physics.velocity.x += physics.forceAccum.x * dt;
    physics.velocity.y += physics.forceAccum.y * dt;
    physics.forceAccum = {};

#if 1
    physics.velocity.x *= (1.0f - physics.drag * dt);
    physics.velocity.y *= (1.0f - physics.drag * dt);
#else
    physics.velocity.x *= 1.0f - powf(physics.drag, dt);
    physics.velocity.y *= 1.0f - powf(physics.drag, dt);
#endif

    entity.position.x += physics.velocity.x * dt;
    entity.position.y += physics.velocity.y * dt;
}
void Tilemap::ResolveEntityTerrainCollisions(uint32_t entityId)
{
    Entity &entity = entities[entityId];
    if (!entity.type) {
        return;
    }

    AspectCollision &eCollision = collision[entityId];
    if (!eCollision.radius) {
        return;
    }

    eCollision.colliding = false;

    Vector2 topLeft{
        entity.position.x - eCollision.radius,
        entity.position.y - eCollision.radius
    };
    Vector2 bottomRight{
        entity.position.x + eCollision.radius,
        entity.position.y + eCollision.radius
    };

    int yMin = CLAMP(floorf(topLeft.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf(bottomRight.y / TILE_W), 0, height);
    int xMin = CLAMP(floorf(topLeft.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf(bottomRight.x / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile{};
            if (AtTry(x, y, tile)) {
                const TileDef &tileDef = GetTileDef(tile);
                if (tileDef.collide) {
                    Rectangle tileRect{};
                    Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    tileRect.x = tilePos.x;
                    tileRect.y = tilePos.y;
                    tileRect.width = TILE_W;
                    tileRect.height = TILE_W;
                    Manifold manifold{};
                    if (dlb_CheckCollisionCircleRec(entity.position, eCollision.radius, tileRect, &manifold)) {
                        eCollision.colliding = true;
                        AspectPhysics &ePhysics = physics[entityId];
                        if (Vector2DotProduct(ePhysics.velocity, manifold.normal) < 0) {
                            entity.position.x += manifold.normal.x * manifold.depth;
                            entity.position.y += manifold.normal.y * manifold.depth;
                        }
                    }
                }
            }
        }
    }
}
void Tilemap::ResolveEntityWarpCollisions(uint32_t entityId, double now)
{
    Entity &entity = entities[entityId];
    if (entity.type != Entity_Player) {
        return;
    }

    AspectCollision &eCollision = collision[entityId];
    if (!eCollision.radius) {
        return;
    }

    for (Warp warp : warps) {
        if (dlb_CheckCollisionCircleRec(entity.position, eCollision.radius, warp.collider, 0)) {
            if (warp.destMap.size()) {
                Err err = Load(warp.destMap, now);
                if (err) {
                    assert(!"UH-OH");
                    exit(EXIT_FAILURE);
                }
            } else {
                Err err = Load(warp.templateMap, now);
                if (err) {
                    assert(!"UH-OH");
                    exit(EXIT_FAILURE);
                }

#if 0
                // TODO: Make a copy of the template map if you wanna edit it
                err = Save(warp.destMap);
                if (err) {
                    assert(!"UH-OH");
                    exit(EXIT_FAILURE);
                }
#endif

                WangTileset wangTileset{};
                err = wangTileset.Load(*this, warp.templateTileset);
                if (err) {
                    assert(!"UH-OH");
                    exit(EXIT_FAILURE);
                }

                WangMap wangMap{};
                err = wangTileset.GenerateMap(width, height, *this, wangMap);
                if (err) {
                    assert(!"UH-OH");
                    exit(EXIT_FAILURE);
                }

                SetFromWangMap(wangMap, now);
            }
            break;
        }
    }
}

void Tilemap::DrawTile(Tile tile, Vector2 position)
{
    const Texture tex = rnTextureCatalog.GetTexture(textureId);
    const Rectangle texRect = TileDefRect(tile);
    DrawTextureRec(tex, texRect, position, WHITE);
}
void Tilemap::Draw(Camera2D &camera)
{
    Rectangle screenRect = GetScreenRectWorld(camera);
    int yMin = CLAMP(floorf(screenRect.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((screenRect.y + screenRect.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(screenRect.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((screenRect.x + screenRect.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile = At(x, y);
            DrawTile(tile, { (float)x * TILE_W, (float)y * TILE_W });
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
            const TileDef &tileDef = GetTileDef(tile);
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
            DrawTextEx(fntHackBold20, TextFormat("%d", tile), tilePos, fntHackBold20.baseSize * (0.5f / camera.zoom), 1 / camera.zoom, WHITE);
        }
    }
}
void Tilemap::DrawEntityHoverInfo(uint32_t entityId)
{
    AspectLife &life = this->life[entityId];
    if (life.maxHealth) {
        const float borderWidth = 1;
        const float pad = 1;
        Vector2 hpBarPad{ pad, pad };
        Vector2 hpBarSize{ 200, 24 };

        Rectangle hpBarBg{
            (float)GetRenderWidth() / 2 - hpBarSize.x / 2 - hpBarPad.x,
            20.0f,
            hpBarSize.x + hpBarPad.x * 2,
            hpBarSize.y + hpBarPad.y * 2
        };

        Rectangle hpBar{
            hpBarBg.x + hpBarPad.x,
            hpBarBg.y + hpBarPad.y,
            hpBarSize.x,
            hpBarSize.y
        };

        DrawRectangleRec(hpBarBg, Fade(BLACK, 0.5));
        float pctHealth = CLAMP((float)life.health / life.maxHealth, 0, 1);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctHealth), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));

        Vector2 labelSize = MeasureTextEx(fntHackBold20, "Lily", fntHackBold20.baseSize, 1);
        Vector2 labelPos{
            floorf(hpBarBg.x + hpBarBg.width / 2 - labelSize.x / 2),
            floorf(hpBarBg.y + hpBarBg.height / 2 - labelSize.y / 2)
        };
        DrawTextShadowEx(fntHackBold20, "Lily", labelPos, WHITE);
    }
}
void Tilemap::DrawEntity(uint32_t entityId, double now) {
    const Entity &entity = entities[entityId];
    const data::Sprite &sprite = this->sprite[entityId];

    const Rectangle rect = EntityRect(entityId);
    if (entity.type == Entity_Player) {
        printf("");
    }
    data::DrawSprite(sprite, { rect.x, rect.y });
}