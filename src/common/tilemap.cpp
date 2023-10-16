#include "collision.h"
#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "texture_catalog.h"
#include "wang.h"

void data::Tilemap::SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y)
{
    strncpy(tileChunk.map_id, id.c_str(), SV_MAX_TILE_MAP_NAME_LEN);
    tileChunk.x = x;
    tileChunk.y = y;
    for (uint32_t ty = y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
        for (uint32_t tx = x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
            AtTry(tx, ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx]);
        }
    }
}
void data::Tilemap::CL_DeserializeChunk(Msg_S_TileChunk &tileChunk)
{
    if (id == std::string(tileChunk.map_id)) {
        for (uint32_t ty = tileChunk.y; ty < SV_TILE_CHUNK_WIDTH; ty++) {
            for (uint32_t tx = tileChunk.x; tx < SV_TILE_CHUNK_WIDTH; tx++) {
                Set(tileChunk.x + tx, tileChunk.y + ty, tileChunk.tileDefs[ty * SV_TILE_CHUNK_WIDTH + tx], 0);
            }
        }
    } else {
        printf("[tilemap] Failed to deserialize chunk with mapId %s into map with id %s\n", tileChunk.map_id, id.c_str());
    }
}

Tile data::Tilemap::At(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return tiles[(size_t)y * width + x];
}
bool data::Tilemap::AtTry(uint32_t x, uint32_t y, Tile &tile)
{
    if (x < width && y < height) {
        tile = At(x, y);
        return true;
    }
    return false;
}
bool data::Tilemap::WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord)
{
    if (world_x < width * TILE_W && world_y < height * TILE_W) {
        coord.x = world_x / TILE_W;
        coord.y = world_y / TILE_W;
        return true;
    }
    return false;
}
bool data::Tilemap::AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile = At(coord.x, coord.y);
        return true;
    }
    return false;
}

void data::Tilemap::Set(uint32_t x, uint32_t y, Tile tile, double now)
{
    assert(x < width);
    assert(y < height);
    Tile &mapTile = tiles[(size_t)y * width + x];
    if (mapTile != tile) {
        mapTile = tile;
        chunkLastUpdatedAt = now;
    }
}
void data::Tilemap::SetFromWangMap(WangMap &wangMap, double now)
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
bool data::Tilemap::NeedsFill(uint32_t x, uint32_t y, int tileDefFill)
{
    Tile tile;
    if (AtTry(x, y, tile)) {
        return tile == tileDefFill;
    }
    return false;
}
void data::Tilemap::Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Coord> &stack)
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
void data::Tilemap::Fill(uint32_t x, uint32_t y, int tileDefId, double now)
{
    Tile tileDefFill = At(x, y);
    if (tileDefFill == tileDefId) {
        return;
    }

    std::stack<data::Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        data::Tilemap::Coord coord = stack.top();
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

const data::GfxFrame &data::Tilemap::GetTileGfxFrame(Tile tile)
{
    const data::TileDef &tile_def = GetTileDef(tile);
    const data::GfxAnim &gfx_anim = data::packs[0]->FindGraphicAnim(tile_def.anim);
    const data::GfxFrame &gfx_frame = data::packs[0]->FindGraphicFrame(gfx_anim.frames[tile_def.anim_state.frame]);
    return gfx_frame;
}
const data::TileDef &data::Tilemap::GetTileDef(Tile tile)
{
    if (tile >= tileDefs.size()) tile = 0;
    return tileDefs[tile];
}
Rectangle data::Tilemap::TileDefRect(Tile tile)
{
    const data::GfxFrame &gfx_frame = GetTileGfxFrame(tile);
    const Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    return rect;
}
Color data::Tilemap::TileDefAvgColor(Tile tile)
{
    const data::TileDef &tile_def = GetTileDef(tile);
    return tile_def.color;
}

data::AiPath *data::Tilemap::GetPath(uint32_t pathId) {
    if (pathId < paths.size()) {
        return &paths[pathId];
    }
    return 0;
}
uint32_t data::Tilemap::GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
    data::AiPath *path = GetPath(pathId);
    if (path) {
        uint32_t nextPathNodeIndex = pathNodeIndex + 1;
        if (nextPathNodeIndex >= path->pathNodeStart + path->pathNodeCount) {
            nextPathNodeIndex = path->pathNodeStart;
        }
        return nextPathNodeIndex;
    }
    return 0;
}
data::AiPathNode *data::Tilemap::GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
    data::AiPath *path = GetPath(pathId);
    if (path) {
        return &pathNodes[pathNodeIndex];
    }
    return 0;
}

void data::Tilemap::ResolveEntityTerrainCollisions(data::Entity &entity)
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

    int yMin = CLAMP(floorf(topLeft.y / TILE_W) - 1, 0, height);
    int yMax = CLAMP(ceilf(bottomRight.y / TILE_W) + 1, 0, height);
    int xMin = CLAMP(floorf(topLeft.x / TILE_W) - 1, 0, width);
    int xMax = CLAMP(ceilf(bottomRight.x / TILE_W) + 1, 0, width);

    for (int iters = 0; iters < 1; iters++) {
        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                Tile tile{};
                if (AtTry(x, y, tile)) {
                    const data::TileDef &tileDef = GetTileDef(tile);
                    const data::Material &material = data::packs[0]->FindMaterial(tileDef.material);
                    if (!material.flags) {
                        Rectangle tileRect{};
                        Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                        tileRect.x = tilePos.x;
                        tileRect.y = tilePos.y;
                        tileRect.width = TILE_W;
                        tileRect.height = TILE_W;
                        Manifold manifold{};
                        if (dlb_CheckCollisionCircleRec(entity.ScreenPos(), entity.radius, tileRect, &manifold)) {
                            const Vector2 resolve = Vector2Scale(manifold.normal, manifold.depth);
                            entity.position.x += resolve.x;
                            entity.position.y += resolve.y;
                            if (resolve.x) entity.velocity.x *= 0.5f;
                            if (resolve.y) entity.velocity.y *= 0.5f;
                            entity.colliding = true;
                        }
                    }
                }
            }
        }
    }
}
void data::Tilemap::ResolveEntityTerrainCollisions(uint32_t entityId)
{
    assert(entityId);
    data::Entity *entity = entityDb->FindEntity(entityId);
    if (!entity || !entity->Alive() || !entity->radius) {
        return;
    }

    ResolveEntityTerrainCollisions(*entity);
}

void data::Tilemap::UpdateAnimations(double dt)
{
    for (TileDef &tile_def : tileDefs) {
        const GfxAnim &anim = packs[0]->FindGraphicAnim(tile_def.anim);
        data::UpdateGfxAnim(anim, dt, tile_def.anim_state);
    }
}

void data::Tilemap::DrawTile(Tile tile, Vector2 position)
{
    // TODO: Yikes.. that's a lot of lookups in a tight loop. Memoize some pointers or something man.
    const data::GfxFrame &gfx_frame = GetTileGfxFrame(tile);
    const GfxFile &gfx_file = data::packs[0]->FindGraphic(gfx_frame.gfx);
    const Rectangle texRect = TileDefRect(tile);

    //position.x = floorf(position.x);
    //position.y = floorf(position.y);
    if (position.x != floorf(position.x)) assert(!"floating x");
    if (position.y != floorf(position.y)) assert(!"floating y");
    dlb_DrawTextureRec(gfx_file.texture, texRect, position, WHITE);
}
void data::Tilemap::Draw(Camera2D &camera)
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
void data::Tilemap::DrawColliders(Camera2D &camera)
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
            const data::Material &material = data::packs[0]->FindMaterial(tileDef.material);
            if (!material.flags) {
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
void data::Tilemap::DrawTileIds(Camera2D &camera)
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
