#include "collision.h"
#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "texture_catalog.h"
#include "wang.h"

uint32_t data::Tilemap::At(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return tiles[(size_t)y * width + x];
}
uint32_t data::Tilemap::At_Obj(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return objects[(size_t)y * width + x];
}
bool data::Tilemap::AtTry(uint32_t x, uint32_t y, uint32_t &tile_id)
{
    if (x < width && y < height) {
        tile_id = At(x, y);
        return true;
    }
    return false;
}
bool data::Tilemap::AtTry_Obj(uint32_t x, uint32_t y, uint32_t &obj_id)
{
    if (x < width && y < height) {
        obj_id = At_Obj(x, y);
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
bool data::Tilemap::AtWorld(uint32_t world_x, uint32_t world_y, uint32_t &tile_id)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile_id = At(coord.x, coord.y);
        return true;
    }
    return false;
}

void data::Tilemap::Set(uint32_t x, uint32_t y, uint32_t tile_id, double now)
{
    assert(x < width);
    assert(y < height);
    uint32_t &cur_tile_id = tiles[(size_t)y * width + x];
    if (cur_tile_id != tile_id) {
        cur_tile_id = tile_id;
        chunkLastUpdatedAt = now;
    }
}
void data::Tilemap::Set_Obj(uint32_t x, uint32_t y, uint32_t object_id, double now)
{
    assert(x < width);
    assert(y < height);
    uint32_t &cur_object_id = objects[(size_t)y * width + x];
    if (cur_object_id != object_id) {
        cur_object_id = object_id;
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
bool data::Tilemap::NeedsFill(uint32_t x, uint32_t y, uint32_t old_tile_id)
{
    uint32_t tile_id;
    if (AtTry(x, y, tile_id)) {
        return tile_id == old_tile_id;
    }
    return false;
}
void data::Tilemap::Scan(uint32_t lx, uint32_t rx, uint32_t y, uint32_t old_tile_id, std::stack<Coord> &stack)
{
    bool inSpan = false;
    for (uint32_t x = lx; x < rx; x++) {
        if (!NeedsFill(x, y, old_tile_id)) {
            inSpan = false;
        } else if (!inSpan) {
            stack.push({ x, y });
            inSpan = true;
        }
    }
}
void data::Tilemap::Fill(uint32_t x, uint32_t y, uint32_t new_tile_id, double now)
{
    uint32_t old_tile_id = At(x, y);
    if (old_tile_id == new_tile_id) {
        return;
    }

    std::stack<data::Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        data::Tilemap::Coord coord = stack.top();
        stack.pop();

        uint32_t lx = coord.x;
        uint32_t rx = coord.x;
        while (lx && NeedsFill(lx - 1, coord.y, old_tile_id)) {
            Set(lx - 1, coord.y, new_tile_id, now);
            lx -= 1;
        }
        while (NeedsFill(rx, coord.y, old_tile_id)) {
            Set(rx, coord.y, new_tile_id, now);
            rx += 1;
        }
        if (coord.y) {
            Scan(lx, rx, coord.y - 1, old_tile_id, stack);
        }
        Scan(lx, rx, coord.y + 1, old_tile_id, stack);
    }
}

data::TileDef &data::Tilemap::GetTileDef(uint32_t tile_id)
{
    data::TileDef &tile_def = data::packs[0]->FindTileDefById(tile_id);
    return tile_def;
}
const data::GfxFrame &data::Tilemap::GetTileGfxFrame(uint32_t tile_id)
{
    const data::TileDef &tile_def = GetTileDef(tile_id);
    const data::GfxAnim &gfx_anim = data::packs[0]->FindGraphicAnim(tile_def.anim);
    const std::string &gfx_frame_id = gfx_anim.GetFrame(tile_def.anim_state.frame);
    const data::GfxFrame &gfx_frame = data::packs[0]->FindGraphicFrame(gfx_frame_id);
    return gfx_frame;
}
Rectangle data::Tilemap::TileDefRect(uint32_t tile_id)
{
    const data::GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    return rect;
}
Color data::Tilemap::TileDefAvgColor(uint32_t tile_id)
{
    const data::TileDef &tile_def = GetTileDef(tile_id);
    return tile_def.color;
}

data::ObjectData *data::Tilemap::GetObjectData(uint32_t x, uint32_t y)
{
    for (data::ObjectData &obj_data : object_data) {
        if (obj_data.x == x && obj_data.y == y) {
            return &obj_data;
        }
    }
    return 0;
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

void data::Tilemap::ResolveEntityCollisions(data::Entity &entity)
{
    if (!entity.radius || entity.Dead()) {
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

    int yMin = floorf(topLeft.y / TILE_W) - 1;
    int yMax = ceilf(bottomRight.y / TILE_W) + 1;
    int xMin = floorf(topLeft.x / TILE_W) - 1;
    int xMax = ceilf(bottomRight.x / TILE_W) + 1;

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            bool solid = true;

            // Out of bounds tiles are considered solid tiles
            if (x >= 0 && x < width && y >= 0 && y < height) {
                const uint32_t tile_id = At(x, y);
                const data::TileDef &tileDef = GetTileDef(tile_id);
                solid = tileDef.flags & data::TILEDEF_FLAG_SOLID;
                if (!solid) {
                    const uint8_t obj = At_Obj(x, y);
                    // NOTE(dlb): Don't collide with void objects like we do with ground tiles
                    if (obj) {
                        const data::TileDef &objTileDef = GetTileDef(obj);
                        solid = objTileDef.flags & data::TILEDEF_FLAG_SOLID;
                    }
                }
            }

            if (solid) {
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

    if (entity.type == data::ENTITY_PLAYER) {
        assert(entity.radius);

        entity.on_warp = false;
        entity.on_warp_coord = {};
        for (int y = yMin; y < yMax && !entity.on_warp; y++) {
            for (int x = xMin; x < xMax && !entity.on_warp; x++) {
                const data::ObjectData *object = GetObjectData(x, y);
                if (object && object->type == "warp") {
                    Rectangle tileRect{};
                    Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    tileRect.x = tilePos.x;
                    tileRect.y = tilePos.y;
                    tileRect.width = TILE_W;
                    tileRect.height = TILE_W;

                    if (dlb_CheckCollisionCircleRec(entity.ScreenPos(), entity.radius, tileRect, 0)) {
                        entity.on_warp = true;
                        entity.on_warp_coord.x = object->x;
                        entity.on_warp_coord.y = object->y;
                    }
                }
            }
        }
    }

    if (!entity.on_warp) {
        entity.on_warp_cooldown = false;  // ready to warp again
    }
}

void data::Tilemap::DrawTile(uint32_t tile_id, Vector2 position, data::DrawCmdQueue *sortedDraws)
{
    // TODO: Yikes.. that's a lot of lookups in a tight loop. Memoize some pointers or something man.
    const data::GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const GfxFile &gfx_file = data::packs[0]->FindGraphic(gfx_frame.gfx);
    const Rectangle texRect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };

    if (sortedDraws) {
        DrawCmd cmd{};
        cmd.texture = gfx_file.texture;
        cmd.rect = texRect;
        cmd.position = { position.x, position.y, 0 };
        cmd.color = WHITE;
        sortedDraws->push(cmd);
    } else {
        dlb_DrawTextureRec(gfx_file.texture, texRect, position, WHITE);
    }
}
void data::Tilemap::Draw(Camera2D &camera, data::DrawCmdQueue &sortedDraws)
{
    const Vector2 cursorWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    int yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint32_t tile_id = At(x, y);
            DrawTile(tile_id, { (float)x * TILE_W, (float)y * TILE_W }, 0);
        }
    }

    // NOTE(dlb): Give obj layer an extra padding on all sides to prevent culling 2-tall things
    yMin = CLAMP(-1 + floorf(cameraRectWorld.y / TILE_W), 0, height);
    yMax = CLAMP( 1 + ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    xMin = CLAMP(-1 + floorf(cameraRectWorld.x / TILE_W), 0, width);
    xMax = CLAMP( 1 + ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);
    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint32_t object_id = At_Obj(x, y);
            if (object_id) {
                DrawTile(object_id, { (float)x * TILE_W, (float)y * TILE_W }, &sortedDraws);
            }
        }
    }
}
void data::Tilemap::DrawColliders(Camera2D &camera)
{
    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    int yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint32_t tile_id = At(x, y);
            const data::TileDef &tileDef = GetTileDef(tile_id);
            if (tileDef.flags & data::TILEDEF_FLAG_SOLID) {
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
    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    int yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    const int pad = 8;
    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint32_t tile_id = At(x, y);
            Vector2 pos = { (float)x * TILE_W + pad, (float)y * TILE_W + pad };
            DrawTextEx(fntSmall, TextFormat("%d", tile_id), pos, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
        }
    }
}
