#include "collision.h"
#include "file_utils.h"
#include "texture_catalog.h"
#include "tilemap.h"
#include "wang.h"

void Tilemap::SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y)
{
    for (uint32_t ty = y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
        for (uint32_t tx = x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
            AtTry(tx, ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx]);
        }
    }
}

void Tilemap::CL_DeserializeChunk(Msg_S_TileChunk &tileChunk)
{
    for (uint32_t ty = tileChunk.y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
        for (uint32_t tx = tileChunk.x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
            Set(tx, ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx], 0);
        }
    }
}

static TileDef v2TileDefs[] = {
#define TILEDEF(y, x, collide) { x * (TILE_W + 2) + 1, y * (TILE_W + 2) + 1, collide }
    TILEDEF(0, 0, 0),  // void
    TILEDEF(0, 1, 0),  // bright grass
    TILEDEF(0, 2, 1),  // bright water
    TILEDEF(0, 3, 0),  // tall grass
    TILEDEF(0, 4, 0),  // cracked sand
    TILEDEF(1, 0, 0),  // light stone path
    TILEDEF(1, 1, 0),  // ugly grass 1
    TILEDEF(1, 2, 0),  // ugly grass 2
    TILEDEF(1, 3, 1),  // tower TL
    TILEDEF(1, 4, 1),  // tower TM
    TILEDEF(2, 0, 1),  // tower TR
    TILEDEF(2, 1, 1),  // ugly bush
    TILEDEF(2, 2, 0),  // grass w/ flowers
    TILEDEF(2, 3, 0),  // dirt
    TILEDEF(2, 4, 0),  // d2 grass
    TILEDEF(3, 0, 0),  // dark grass
    TILEDEF(3, 1, 1),  // tower ML
    TILEDEF(3, 2, 1),  // tower MM
    TILEDEF(3, 3, 1),  // tower MR
    TILEDEF(3, 4, 1),  // dark water
#undef TILEDEF
};

static AiPathNode v2HardcodedAiPathNodes[] = {
    { 554, 347 + 800, 0 },
    { 598, 408 + 800, 0 },
    { 673, 450 + 800, 0 },
    { 726, 480 + 800, 0 },
    { 767, 535 + 800, 3 },
    { 813, 595 + 800, 0 },
    { 888, 621 + 800, 0 },
    { 952, 598 + 800, 0 },
    { 990, 553 + 800, 0 },
    { 999, 490 + 800, 0 },
    { 988, 426 + 800, 0 },
    { 949, 368 + 800, 0 },
    { 905, 316 + 800, 1 },
    { 857, 260 + 800, 0 },
    { 801, 239 + 800, 0 },
    { 757, 267 + 800, 0 },
    { 702, 295 + 800, 2 },
    { 641, 274 + 800, 0 },
    { 591, 258 + 800, 0 },
    { 546, 289 + 800, 0 }
};

static uint32_t v2HardcodedAiPathNodeIndices[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
};

static AiPath v2HardcodedAiPaths[] = {
    { 0, 20 }
};

Tilemap::~Tilemap(void)
{
    Unload();
}

Err Tilemap::Save(const char *filename)
{
    Err err = RN_SUCCESS;

    if (FileExists(filename)) {
        err = MakeBackup(filename);
        if (err) return err;
    }

    FILE *file = fopen(filename, "w");
    do {
        if (!file) {
            err = RN_BAD_FILE_WRITE; break;
        }

        assert(texturePath);
        assert(tileDefCount);
        assert(width);
        assert(height);
        assert(pathNodeCount);
        assert(pathNodeIndexCount);
        assert(pathCount);

        assert(tileDefs);
        assert(tiles);
        assert(pathNodes);
        assert(pathNodeIndices);
        assert(paths);

        fwrite(&MAGIC, sizeof(MAGIC), 1, file);
        fwrite(&VERSION, sizeof(VERSION), 1, file);
        int texturePathLen = strlen(texturePath);
        fwrite(&texturePathLen, sizeof(texturePathLen), 1, file);
        fwrite(texturePath, sizeof(*texturePath), texturePathLen, file);
        fwrite(&width, sizeof(width), 1, file);
        fwrite(&height, sizeof(height), 1, file);
        fwrite(&pathNodeCount, sizeof(pathNodeCount), 1, file);
        fwrite(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
        fwrite(&pathCount, sizeof(pathCount), 1, file);

        for (uint32_t i = 0; i < tileDefCount; i++) {
            TileDef &tileDef = tileDefs[i];
            fwrite(&tileDef.collide, sizeof(tileDef.collide), 1, file);
        }

        const uint32_t tileCount = width * height;
        for (uint32_t i = 0; i < tileCount; i++) {
            Tile &tile = tiles[i];
            fwrite(&tiles[i], sizeof(tiles[i]), 1, file);
        }

        for (uint32_t i = 0; i < pathNodeCount; i++) {
            AiPathNode &aiPathNode = pathNodes[i];
            fwrite(&aiPathNode.pos.x, sizeof(aiPathNode.pos.x), 1, file);
            fwrite(&aiPathNode.pos.y, sizeof(aiPathNode.pos.y), 1, file);
            fwrite(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
        }

        for (uint32_t i = 0; i < pathNodeIndexCount; i++) {
            uint32_t &aiPathNodeIndex = pathNodeIndices[i];
            fwrite(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
        }

        for (uint32_t i = 0; i < pathCount; i++) {
            AiPath &aiPath = paths[i];
            fwrite(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
            fwrite(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
        }
    } while (0);

    if (file) fclose(file);
    if (!err) this->filename = filename;
    return RN_SUCCESS;
}

// TODO(dlb): This shouldn't load in-place.. it causes the game to crash when
// the load fails. Instead, return a new a tilemap and switch over to it after
// it has been loaded successfully.
Err Tilemap::Load(const char *filename, double now)
{
    Err err = RN_SUCCESS;

    Unload();

    FILE *file = fopen(filename, "r");
    Image img{};
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
        if (version >= 2) {
            fread(&texturePathLen, sizeof(texturePathLen), 1, file);
            if (!texturePathLen || texturePathLen > 1024) {
                err = RN_INVALID_PATH; break;
            }
            texturePath = (char *)calloc((size_t)texturePathLen + 1, sizeof(*texturePath));
            if (!texturePath) {
                err = RN_BAD_ALLOC; break;
            }
            fread(texturePath, sizeof(*texturePath), texturePathLen, file);
        } else {
            const char *v1_texturePath = "resources/tiles32.png";
            texturePathLen = (uint32_t)strlen(v1_texturePath);
            texturePath = (char *)calloc((size_t)texturePathLen + 1, sizeof(*texturePath));
            if (!texturePath) {
                err = RN_BAD_ALLOC; break;
            }
            strncpy(texturePath, v1_texturePath, texturePathLen);
        }

        textureId = rnTextureCatalog.FindOrLoad(texturePath);
        Texture texture = rnTextureCatalog.ById(textureId);
        if (texture.width % TILE_W != 0 || texture.height % TILE_W != 0) {
            err = RN_INVALID_SIZE; break;
        }

        tileDefCount = (texture.width / TILE_W) * (texture.height / TILE_W);
        int v3TileDefCount = 0;
        if (version == 3) {
            fread(&v3TileDefCount, sizeof(v3TileDefCount), 1, file);
        }

        fread(&width, sizeof(width), 1, file);
        fread(&height, sizeof(height), 1, file);
        if (!width || !height) {
            err = RN_INVALID_SIZE; break;
        }

        if (version >= 3) {
            fread(&pathNodeCount, sizeof(pathNodeCount), 1, file);
            fread(&pathNodeIndexCount, sizeof(pathNodeIndexCount), 1, file);
            fread(&pathCount, sizeof(pathCount), 1, file);
        } else {
            pathNodeCount = ARRAY_SIZE(v2HardcodedAiPathNodes);
            pathNodeIndexCount = ARRAY_SIZE(v2HardcodedAiPathNodeIndices);
            pathCount = ARRAY_SIZE(v2HardcodedAiPaths);
        }

        tileDefs = (TileDef *)calloc(tileDefCount, sizeof(*tileDefs));
        if (!tileDefs) {
            err = RN_BAD_ALLOC; break;
        }

        if (version >= 4) {
            for (uint32_t i = 0; i < tileDefCount; i++) {
                TileDef &dst = tileDefs[i];
                int tilesPerRow = texture.width / TILE_W;
                dst.x = i % tilesPerRow * TILE_W;
                dst.y = i / tilesPerRow * TILE_W;
                fread(&dst.collide, sizeof(dst.collide), 1, file);
            }
        } else if (version >= 3) {
            for (uint32_t i = 0; i < v3TileDefCount; i++) {
                TileDef &dst = tileDefs[i];
                uint32_t v3IgnoreX;
                fread(&v3IgnoreX, sizeof(v3IgnoreX), 1, file);
                uint32_t v3IgnoreY;
                fread(&v3IgnoreY, sizeof(v3IgnoreY), 1, file);

                int tilesPerRow = texture.width / TILE_W;
                dst.x = i % tilesPerRow * TILE_W;
                dst.y = i / tilesPerRow * TILE_W;
                fread(&dst.collide, sizeof(dst.collide), 1, file);
            }
        } else {
            for (uint32_t i = 0; i < tileDefCount; i++) {
                TileDef &src = v2TileDefs[i];
                TileDef &dst = tileDefs[i];
                //dst.x = src.x;
                //dst.y = src.y;

                int tilesPerRow = texture.width / TILE_W;
                dst.x = i % tilesPerRow * TILE_W;
                dst.y = i / tilesPerRow * TILE_W;
                dst.collide = src.collide;
            }
        }

        const uint32_t tileCount = width * height;
        tiles = (Tile *)calloc(tileCount, sizeof(*tiles));
        if (!tiles) {
            err = RN_BAD_ALLOC; break;
        }
        for (uint32_t i = 0; i < tileCount; i++) {
            Tile &tile = tiles[i];
            fread(&tiles[i], sizeof(tiles[i]), 1, file);
            if (tiles[i] >= tileDefCount) {
                err = RN_OUT_OF_BOUNDS; break;
            }
        }
        chunkLastUpdatedAt = now;

        pathNodes = (AiPathNode *)calloc(pathNodeCount, sizeof(*pathNodes));
        if (!pathNodes) {
            err = RN_BAD_ALLOC; break;
        }
        if (version >= 3) {
            for (uint32_t i = 0; i < pathNodeCount; i++) {
                AiPathNode &aiPathNode = pathNodes[i];
                fread(&aiPathNode.pos.x, sizeof(aiPathNode.pos.x), 1, file);
                fread(&aiPathNode.pos.y, sizeof(aiPathNode.pos.y), 1, file);
                fread(&aiPathNode.waitFor, sizeof(aiPathNode.waitFor), 1, file);
            }
        } else {
            for (uint32_t i = 0; i < pathNodeCount; i++) {
                AiPathNode &aiPathNode = pathNodes[i];
                aiPathNode.pos.x = v2HardcodedAiPathNodes[i].pos.x;
                aiPathNode.pos.y = v2HardcodedAiPathNodes[i].pos.y;
                aiPathNode.waitFor = v2HardcodedAiPathNodes[i].waitFor;
            }
        }

        pathNodeIndices = (uint32_t *)calloc(pathNodeIndexCount, sizeof(*pathNodeIndices));
        if (!pathNodeIndices) {
            err = RN_BAD_ALLOC; break;
        }
        if (version >= 3) {
            for (uint32_t i = 0; i < pathNodeIndexCount; i++) {
                uint32_t &aiPathNodeIndex = pathNodeIndices[i];
                fread(&aiPathNodeIndex, sizeof(aiPathNodeIndex), 1, file);
            }
        } else {
            for (uint32_t i = 0; i < pathNodeIndexCount; i++) {
                uint32_t &aiPathNodeIndex = pathNodeIndices[i];
                aiPathNodeIndex = v2HardcodedAiPathNodeIndices[i];
            }
        }

        paths = (AiPath *)calloc(pathCount, sizeof(*paths));
        if (!paths) {
            err = RN_BAD_ALLOC; break;
        }
        if (version >= 3) {
            for (uint32_t i = 0; i < pathCount; i++) {
                AiPath &aiPath = paths[i];
                fread(&aiPath.pathNodeIndexOffset, sizeof(aiPath.pathNodeIndexOffset), 1, file);
                fread(&aiPath.pathNodeIndexCount, sizeof(aiPath.pathNodeIndexCount), 1, file);
            }
        } else {
            for (uint32_t i = 0; i < pathCount; i++) {
                AiPath &aiPath = paths[i];
                aiPath.pathNodeIndexOffset = v2HardcodedAiPaths[i].pathNodeIndexOffset;
                aiPath.pathNodeIndexCount = v2HardcodedAiPaths[i].pathNodeIndexCount;
            }
        }

        img = LoadImage(texturePath);
        if (!img.width) {
            err = RN_BAD_FILE_READ; break;
        }
        for (int i = 0; i < tileDefCount; i++) {
            TileDef &tileDef = tileDefs[i];
            assert(tileDef.x < img.width);
            assert(tileDef.y < img.height);
            tileDef.color = GetImageColor(img, tileDef.x, tileDef.y);
        }
    } while (0);
    if (img.data) UnloadImage(img);
    if (file) fclose(file);

    if (!err) {
        this->filename = filename;
    } else {
        Unload();
    }
    return err;
}

Err Tilemap::ChangeTileset(Tilemap &map, const char *newTexturePath, double now)
{
    Err err = RN_SUCCESS;

    if (!strcmp(map.texturePath, newTexturePath)) {
        assert(!"u wot mate?");
        return RN_SUCCESS;
    }

    Tilemap newMap{};
    Image img{};
    do {
        newMap.textureId = rnTextureCatalog.FindOrLoad(newTexturePath);
        Texture texture = rnTextureCatalog.ById(newMap.textureId);
        if (texture.width % TILE_W != 0 || texture.height % TILE_W != 0) {
            err = RN_INVALID_SIZE; break;
        }
        newMap.tileDefCount = (texture.width / TILE_W) * (texture.height / TILE_W);

        const int texturePathLen = strlen(newTexturePath);
        newMap.texturePath = (char *)calloc((size_t)texturePathLen + 1, sizeof(*newMap.texturePath));
        strncpy(newMap.texturePath, newTexturePath, texturePathLen);

        newMap.tileDefs = (TileDef *)calloc(newMap.tileDefCount, sizeof(*tileDefs));
        for (int i = 0; i < newMap.tileDefCount; i++) {
            TileDef &dst = newMap.tileDefs[i];
            int tilesPerRow = texture.width / TILE_W;
            dst.x = i % tilesPerRow * TILE_W;
            dst.y = i / tilesPerRow * TILE_W;

            // This probably kinda stupid.. but idk
            if (i < map.tileDefCount) {
                TileDef &src = map.tileDefs[i];
                dst.collide = src.collide;
            }
        }

        // TODO(dlb): If we want to be able to change the size of tileset images
        // but still have the old indices somehow resolve to the same art in the
        // new tileset image, we should probably do something fancy like hash
        // the pixel data of the tiles and match them up that way or wutevs.
        //
        // If we did that, maybe it's optional.. because you might want to move
        // tiles around on purpose and have them map to some other tile, right?
        //
        newMap.tiles = (Tile *)calloc((size_t)map.width * map.height, sizeof(*newMap.tiles));
        if (!newMap.tiles) {
            err = RN_BAD_ALLOC; break;
        }
        for (int i = 0; i < map.width * map.height; i++) {
            Tile &src = map.tiles[i];
            Tile &dst = newMap.tiles[i];
            dst = src >= newMap.tileDefCount ? 0 : src;
        }

        img = LoadImage(newMap.texturePath);
        if (!img.width) {
            err = RN_BAD_FILE_READ; break;
        }
        for (int i = 0; i < newMap.tileDefCount; i++) {
            TileDef &tileDef = newMap.tileDefs[i];
            tileDef.color = GetImageColor(img, tileDef.x, tileDef.y);
        }
    } while(0);
    if (img.width) UnloadImage(img);

    if (!err) {
        rnTextureCatalog.Unload(map.texturePath);
        free(map.texturePath);
        free(map.tileDefs);
        free(map.tiles);
        //free(pathNodes);
        //free(pathNodeIndices);
        //free(paths);

        // HACK(dlb): The weird zeroing logic here is so that newMap's
        // destructor does not unload all the new data we just created above.

        map.texturePath = newMap.texturePath;
        newMap.texturePath = 0;
        map.tileDefCount = newMap.tileDefCount;
        //map.width = newMap.width;
        //map.height = newMap.width;
        //map.pathNodeCount = newMap.pathNodeCount;
        //map.pathNodeIndexCount = newMap.pathNodeIndexCount;
        //map.pathCount = newMap.pathCount;
        map.tileDefs = newMap.tileDefs;
        newMap.tileDefs = 0;
        map.tiles = newMap.tiles;
        newMap.tiles = 0;
        //map.pathNodes = newMap.pathNodes;
        //map.pathNodeIndices = newMap.pathNodeIndices;
        //map.paths = newMap.paths;
        map.chunkLastUpdatedAt = now;
    } else {
        newMap.Unload();
    }
    return err;
}

void Tilemap::Unload(void)
{
    rnTextureCatalog.Unload(texturePath);
    free(texturePath);
    free(tileDefs);
    free(tiles);
    free(pathNodes);
    free(pathNodeIndices);
    free(paths);
}

Tile Tilemap::At(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return tiles[y * width + x];
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
    Tile &mapTile = tiles[y * width + x];
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
    for (int y = 0; y < width; y++) {
        for (int x = 0; x < height; x++) {
            uint8_t tile = pixels[y * width + x];
            tile = CLAMP(tile, 0, tileDefCount - 1);
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

void Tilemap::ResolveEntityTerrainCollisions(Entity &entity)
{
    if (!entity.radius) {
        return;
    }

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
                TileDef &tileDef = tileDefs[tile];
                if (tileDef.collide) {
                    Rectangle tileRect{};
                    Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    tileRect.x = tilePos.x;
                    tileRect.y = tilePos.y;
                    tileRect.width = TILE_W;
                    tileRect.height = TILE_W;
                    Manifold manifold{};
                    if (dlb_CheckCollisionCircleRec(entity.position, entity.radius, tileRect, &manifold)) {
                        entity.colliding = true;
                        if (Vector2DotProduct(entity.velocity, manifold.normal) < 0) {
                            entity.position.x += manifold.normal.x * manifold.depth;
                            entity.position.y += manifold.normal.y * manifold.depth;
                        }
                    }
                }
            }
        }
    }
}

Rectangle Tilemap::TileDefRect(Tile tile)
{
    const TileDef &tileDef = tileDefs[tile];
    const Rectangle rect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
    return rect;
}

Color Tilemap::TileDefAvgColor(Tile tile)
{
    const TileDef &tileDef = tileDefs[tile];
    return tileDef.color;
}

void Tilemap::DrawTile(Tile tile, Vector2 position)
{
    const Texture tex = rnTextureCatalog.ById(textureId);
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
            TileDef &tileDef = tileDefs[tile];
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