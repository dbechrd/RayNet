#include "collision.h"
#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "wang.h"

uint32_t Tilemap::At(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return tiles[(size_t)y * width + x];
}
uint32_t Tilemap::At_Obj(uint32_t x, uint32_t y)
{
    assert(x < width);
    assert(y < height);
    return objects[(size_t)y * width + x];
}
bool Tilemap::AtTry(uint32_t x, uint32_t y, uint32_t &tile_id)
{
    if (x < width && y < height) {
        tile_id = At(x, y);
        return true;
    }
    return false;
}
bool Tilemap::AtTry_Obj(uint32_t x, uint32_t y, uint32_t &obj_id)
{
    if (x < width && y < height) {
        obj_id = At_Obj(x, y);
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
bool Tilemap::AtWorld(uint32_t world_x, uint32_t world_y, uint32_t &tile_id)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile_id = At(coord.x, coord.y);
        return true;
    }
    return false;
}
bool Tilemap::IsSolid(int x, int y)
{
    bool solid = true;

    // Out of bounds tiles are considered solid tiles
    if (x >= 0 && x < width && y >= 0 && y < height) {
        const uint32_t tile_id = At(x, y);
        const TileDef &tileDef = GetTileDef(tile_id);
        solid = tileDef.flags & TILEDEF_FLAG_SOLID;
        if (!solid) {
            const uint8_t obj = At_Obj(x, y);
            // NOTE(dlb): Don't collide with void objects like we do with ground tiles
            if (obj) {
                const TileDef &objTileDef = GetTileDef(obj);
                solid = objTileDef.flags & TILEDEF_FLAG_SOLID;
            }
        }
    }

    return solid;
}

void Tilemap::Set(uint32_t x, uint32_t y, uint32_t tile_id, double now)
{
    assert(x < width);
    assert(y < height);
    uint32_t &cur_tile_id = tiles[(size_t)y * width + x];
    if (cur_tile_id != tile_id) {
        cur_tile_id = tile_id;
        dirtyTiles.insert({ x, y });
        chunkLastUpdatedAt = now;
    }
}
void Tilemap::Set_Obj(uint32_t x, uint32_t y, uint32_t object_id, double now)
{
    assert(x < width);
    assert(y < height);
    uint32_t &cur_object_id = objects[(size_t)y * width + x];
    if (cur_object_id != object_id) {
        cur_object_id = object_id;
        dirtyTiles.insert({ x, y });
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
bool Tilemap::NeedsFill(uint32_t x, uint32_t y, uint32_t old_tile_id)
{
    uint32_t tile_id;
    if (AtTry(x, y, tile_id)) {
        return tile_id == old_tile_id;
    }
    return false;
}
void Tilemap::Scan(uint32_t lx, uint32_t rx, uint32_t y, uint32_t old_tile_id, std::stack<Coord> &stack)
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
void Tilemap::Fill(uint32_t x, uint32_t y, uint32_t new_tile_id, double now)
{
    uint32_t old_tile_id = At(x, y);
    if (old_tile_id == new_tile_id) {
        return;
    }

    std::stack<Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Tilemap::Coord coord = stack.top();
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

TileDef &Tilemap::GetTileDef(uint32_t tile_id)
{
    TileDef &tile_def = packs[0].tile_defs[tile_id]; // FindTileDefById(tile_id);
    return tile_def;
}
const GfxFrame &Tilemap::GetTileGfxFrame(uint32_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    const GfxAnim &gfx_anim = packs[0].FindGraphicAnim(tile_def.anim);
    const std::string &gfx_frame_id = gfx_anim.GetFrame(tile_def.anim_state.frame);
    const GfxFrame &gfx_frame = packs[0].FindGraphicFrame(gfx_frame_id);
    return gfx_frame;
}
Rectangle Tilemap::TileDefRect(uint32_t tile_id)
{
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    return rect;
}
Color Tilemap::TileDefAvgColor(uint32_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    return tile_def.color;
}

ObjectData *Tilemap::GetObjectData(uint32_t x, uint32_t y)
{
    for (ObjectData &obj_data : object_data) {
        if (obj_data.x == x && obj_data.y == y) {
            return &obj_data;
        }
    }
    return 0;
}

AiPath *Tilemap::GetPath(uint32_t pathId) {
    // NOTE: The first path is pathId 1 such that "0" can mean no pathing
    if (pathId && pathId <= paths.size()) {
        return &paths[pathId - 1];
    }
    return 0;
}
uint32_t Tilemap::GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path) {
        uint32_t nextPathNodeIndex = pathNodeIndex + 1;
        if (nextPathNodeIndex >= path->pathNodeStart + path->pathNodeCount) {
            nextPathNodeIndex = path->pathNodeStart;
        }
        return nextPathNodeIndex;
    }
    return 0;
}
AiPathNode *Tilemap::GetPathNode(uint32_t pathId, uint32_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path) {
        return &pathNodes[pathNodeIndex];
    }
    return 0;
}

void Tilemap::GetEdges(Edge::Array &edges)
{
    // Clockwise winding, Edge Normal = (-y, x)

    int topStartIdx = -1;
    int bottomStartIdx = -1;
    for (int y = 0; y <= height; y++) {
        for (int x = 0; x <= width; x++) {
            const bool solid = IsSolid(x, y);
            const bool solidAbove = IsSolid(x, y - 1);

            const bool isTopEdge = solid && !solidAbove;
            if (topStartIdx == -1 && isTopEdge) {
                topStartIdx = x;
            } else if (topStartIdx != -1 && (!isTopEdge || x == width )) {
                const Vector2 left { (float)topStartIdx * TILE_W, (float)y * TILE_W };
                const Vector2 right{ (float)x           * TILE_W, (float)y * TILE_W };
                edges.push_back(Edge{{ left, right }});
                topStartIdx = -1;
            }

            const bool isBottomEdge = !solid && solidAbove;
            if (bottomStartIdx == -1 && isBottomEdge) {
                bottomStartIdx = x;
            } else if (bottomStartIdx != -1 && (!isBottomEdge || x == width )) {
                const Vector2 left { (float)bottomStartIdx * TILE_W, (float)y * TILE_W };
                const Vector2 right{ (float)x              * TILE_W, (float)y * TILE_W };
                edges.push_back(Edge{{ right, left }});
                bottomStartIdx = -1;
            }
        }
    }

    int leftStartIdx = -1;
    int rightStartIdx = -1;
    for (int x = 0; x <= width; x++) {
        for (int y = 0; y <= height; y++) {
            const bool solid = IsSolid(x, y);
            const bool solidLeft = IsSolid(x - 1, y);

            const bool isLeftEdge = solid && !solidLeft;
            if (leftStartIdx == -1 && isLeftEdge) {
                leftStartIdx = y;
            } else if (leftStartIdx != -1 && (!isLeftEdge || y == height )) {
                const Vector2 top   { (float)x * TILE_W, (float)leftStartIdx * TILE_W };
                const Vector2 bottom{ (float)x * TILE_W, (float)y            * TILE_W };
                edges.push_back(Edge{{ bottom, top }});
                leftStartIdx = -1;
            }

            const bool isRightEdge = !solid && solidLeft;
            if (rightStartIdx == -1 && isRightEdge) {
                rightStartIdx = y;
            } else if (rightStartIdx != -1 && (!isRightEdge || y == height )) {
                const Vector2 top   { (float)x * TILE_W, (float)rightStartIdx * TILE_W };
                const Vector2 bottom{ (float)x * TILE_W, (float)y             * TILE_W };
                edges.push_back(Edge{{ top, bottom }});
                rightStartIdx = -1;
            }
        }
    }
}
void Tilemap::UpdateEdges(void)
{
    //PerfTimer t{ "UpdateEdges" };
    edges.clear();

    // TODO: Only run this when the edge list is dirty
    GetEdges(edges);
}

void Tilemap::ResolveEntityCollisionsEdges(Entity &entity)
{
    if (!entity.radius || entity.Dead()) {
        return;
    }

    entity.colliding = false;

    std::vector<Collision> collisions{};
    for (Edge &edge : edges) {
        Manifold manifold{};
        if (dlb_CheckCollisionCircleEdge(entity.Position2D(), entity.radius, edge, &manifold)) {
            Collision collision{};
            collision.edge = &edge;
            collision.manifold = manifold;
            collisions.push_back(collision);
        }
    }

    if (collisions.size() > 1) {
        std::sort(collisions.begin(), collisions.end(), std::greater<>{});
    }

    for (Collision &collision : collisions) {
        Manifold new_manifold{};
        if (dlb_CheckCollisionCircleEdge(entity.Position2D(), entity.radius, *collision.edge, &new_manifold)) {
            const Vector2 resolve = Vector2Scale(new_manifold.normal, new_manifold.depth);
            entity.position.x += resolve.x;
            entity.position.y += resolve.y;
        }
    }

    // Hard constraint to keep entity in the map
    entity.position.x = CLAMP(entity.position.x, entity.radius, width * TILE_W - entity.radius);
    entity.position.y = CLAMP(entity.position.y, entity.radius, height * TILE_W - entity.radius);
}
void Tilemap::ResolveEntityCollisionsTriggers(Entity &entity)
{
    if (!entity.radius || entity.Dead()) {
        return;
    }

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

    if (entity.type == ENTITY_PLAYER) {
        assert(entity.radius);

        entity.on_warp = false;
        entity.on_warp_coord = {};
        for (int y = yMin; y < yMax && !entity.on_warp; y++) {
            for (int x = xMin; x < xMax && !entity.on_warp; x++) {
                const ObjectData *object = GetObjectData(x, y);
                if (object && object->type == "warp") {
                    Rectangle tileRect{
                        (float)x * TILE_W,
                        (float)y * TILE_W,
                        TILE_W,
                        TILE_W
                    };

                    if (dlb_CheckCollisionCircleRec(entity.Position2D(), entity.radius, tileRect, 0)) {
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

void Tilemap::DrawTile(uint32_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws)
{
    // TODO: Yikes.. that's a lot of lookups in a tight loop. Memoize some pointers or something man.
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const GfxFile &gfx_file = packs[0].FindGraphic(gfx_frame.gfx);
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
void Tilemap::Draw(Camera2D &camera, DrawCmdQueue &sortedDraws)
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
void Tilemap::DrawColliders(Camera2D &camera)
{
    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    int yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    int xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint32_t tile_id = At(x, y);
            const TileDef &tileDef = GetTileDef(tile_id);
            if (tileDef.flags & TILEDEF_FLAG_SOLID) {
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
void Tilemap::DrawEdges(void)
{
    for (const Edge &edge : edges) {
        edge.Draw(MAGENTA, 3.0f);
    }
}
void Tilemap::DrawTileIds(Camera2D &camera)
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
