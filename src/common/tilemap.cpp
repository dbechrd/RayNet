#include "collision.h"
#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "wang.h"

uint16_t Tilemap::At(TileLayerType layer, uint16_t x, uint16_t y)
{
    assert(x < width);
    assert(y < height);
    return layers[layer][(size_t)y * width + x];
}
bool Tilemap::AtTry(TileLayerType layer, int x, int y, uint16_t &tile_id)
{
    if (x >= 0 && y >= 0 && x < width && y < height) {
        tile_id = At(layer, x, y);
        return true;
    }
    return false;
}
bool Tilemap::WorldToTileIndex(int world_x, int world_y, Coord &coord)
{
    coord.x = world_x / TILE_W;
    coord.y = world_y / TILE_W;
    if (world_x >= 0 && world_y >= 0 && world_x < width * TILE_W && world_y < height * TILE_W) {
        return true;
    }
    return false;
}
bool Tilemap::AtWorld(TileLayerType layer, int world_x, int world_y, uint16_t &tile_id)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile_id = At(layer, coord.x, coord.y);
        return true;
    }
    return false;
}
bool Tilemap::IsSolid(int x, int y)
{
    for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
        uint16_t tile_id = 0;
        if (!AtTry((TileLayerType)layer, x, y, tile_id)) {
            // Out of bounds tiles are considered solid tiles
            return true;
        }

        // NOTE(dlb): Don't collide with void tiles above ground level (it just means there's no object)
        if (layer > 0 && !tile_id) {
            continue;
        }

        const TileDef &tileDef = GetTileDef(tile_id);
        if (tileDef.flags & TileDef::FLAG_SOLID) {
            return true;
        }
    }
    return false;
}

void Tilemap::Set(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now)
{
    assert(x < width);
    assert(y < height);
    uint16_t &cur_tile_id = layers[layer][(size_t)y * width + x];
    if (cur_tile_id != tile_id) {
        cur_tile_id = tile_id;

        // TODO: Don't do this on client, expensive, waste of time
        dirtyTiles.insert({ x, y });
        chunkLastUpdatedAt = now;
    }
}
bool Tilemap::SetTry(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now)
{
    if (x >= 0 && y >= 0 && x < width && y < height) {
        Set(layer, x, y, tile_id, now);
        return true;
    }
    return false;
}
void Tilemap::SetFromWangMap(WangMap &wangMap, double now)
{
    // TODO: Specify map coords to set (or chunk id) and do a bounds check here instead
    if (width != wangMap.image.width || height != wangMap.image.height) {
        return;
    }

    uint8_t *pixels = (uint8_t *)wangMap.image.data;
    for (uint16_t y = 0; y < width; y++) {
        for (uint16_t x = 0; x < height; x++) {
            uint8_t tile = pixels[y * width + x];
            tile = tile < (width * height) ? tile : 0;
            Set(TILE_LAYER_GROUND, x, y, tile, now);
        }
    }
}
bool Tilemap::NeedsFill(TileLayerType layer, uint16_t x, uint16_t y, uint16_t old_tile_id)
{
    uint16_t tile_id{};
    if (AtTry(layer, x, y, tile_id)) {
        return tile_id == old_tile_id;
    }
    return false;
}
void Tilemap::Scan(TileLayerType layer, uint16_t lx, uint16_t rx, uint16_t y, uint16_t old_tile_id, std::stack<Coord> &stack)
{
    bool inSpan = false;
    for (uint16_t x = lx; x < rx; x++) {
        if (!NeedsFill(layer, x, y, old_tile_id)) {
            inSpan = false;
        } else if (!inSpan) {
            stack.push({ x, y });
            inSpan = true;
        }
    }
}
void Tilemap::Fill(TileLayerType layer, uint16_t x, uint16_t y, uint16_t new_tile_id, double now)
{
    uint16_t old_tile_id = At(layer, x, y);
    if (old_tile_id == new_tile_id) {
        return;
    }

    std::stack<Tilemap::Coord> stack{};
    stack.push({ x, y });

    while (!stack.empty()) {
        Tilemap::Coord coord = stack.top();
        stack.pop();

        uint16_t lx = coord.x;
        uint16_t rx = coord.x;
        while (lx && NeedsFill(layer, lx - 1, coord.y, old_tile_id)) {
            Set(layer, lx - 1, coord.y, new_tile_id, now);
            lx -= 1;
        }
        while (NeedsFill(layer, rx, coord.y, old_tile_id)) {
            Set(layer, rx, coord.y, new_tile_id, now);
            rx += 1;
        }
        if (coord.y) {
            Scan(layer, lx, rx, coord.y - 1, old_tile_id, stack);
        }
        Scan(layer, lx, rx, coord.y + 1, old_tile_id, stack);
    }
}

TileDef &Tilemap::GetTileDef(uint16_t tile_id)
{
    if (tile_id < pack_assets.tile_defs.size()) {
         return pack_assets.tile_defs[tile_id];
    }
    return pack_assets.tile_defs[0];
}
const GfxFrame &Tilemap::GetTileGfxFrame(uint16_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    const GfxAnim &gfx_anim = pack_assets.FindByName<GfxAnim>(tile_def.anim);
    const std::string &gfx_frame_id = gfx_anim.GetFrame(tile_def.anim_state.frame);
    const GfxFrame &gfx_frame = pack_assets.FindByName<GfxFrame>(gfx_frame_id);
    return gfx_frame;
}
Rectangle Tilemap::TileDefRect(uint16_t tile_id)
{
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    return rect;
}
Color Tilemap::TileDefAvgColor(uint16_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    return tile_def.color;
}

ObjectData *Tilemap::GetObjectData(uint16_t x, uint16_t y)
{
    for (ObjectData &obj_data : object_data) {
        if (obj_data.x == x && obj_data.y == y) {
            return &obj_data;
        }
    }
    return 0;
}

AiPath *Tilemap::GetPath(uint16_t pathId) {
    // NOTE: The first path is pathId 1 such that "0" can mean no pathing
    if (pathId && pathId <= paths.size()) {
        return &paths[pathId - 1];
    }
    return 0;
}
uint16_t Tilemap::GetNextPathNodeIndex(uint16_t pathId, uint16_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path) {
        uint16_t nextPathNodeIndex = pathNodeIndex + 1;
        if (nextPathNodeIndex >= path->pathNodeStart + path->pathNodeCount) {
            nextPathNodeIndex = path->pathNodeStart;
        }
        return nextPathNodeIndex;
    }
    return 0;
}
AiPathNode *Tilemap::GetPathNode(uint16_t pathId, uint16_t pathNodeIndex) {
    AiPath *path = GetPath(pathId);
    if (path) {
        return &path_nodes[pathNodeIndex];
    }
    return 0;
}

bool IsActivePowerSource(ObjectData &obj)
{
    // TODO: Make object flags or something more efficient
    return obj.power_level && obj.type == OBJ_LEVER;
}

bool IsPowerLoad(ObjectData &obj)
{
    return obj.type == OBJ_DOOR;
}

void Tilemap::UpdatePower(double now)
{
    std::unordered_set<uint8_t> active_power_channels{};

    for (auto &obj : object_data) {
        if (IsActivePowerSource(obj)) {
            active_power_channels.insert(obj.power_channel);
        }
    }

    for (auto &obj : object_data) {
        const int powered = (int)active_power_channels.contains(obj.power_channel);
        if (obj.power_level != powered) {
            if (IsPowerLoad(obj)) {
                obj.power_level = powered;
            }
        }

        uint16_t old_tile_id = At(TILE_LAYER_OBJECT, obj.x, obj.y);
        uint16_t new_tile_id = obj.tile;
        if (obj.power_level) {
            new_tile_id = obj.tile_powered;
        }

        if (new_tile_id != old_tile_id) {
            Set(TILE_LAYER_OBJECT, obj.x, obj.y, new_tile_id, now);
        }
    }
}

void Tilemap::UpdateEdges(Edge::Array &edges)
{
    //PerfTimer t{ "UpdateEdges" };
    edges.clear();

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
void Tilemap::Update(double now, bool simulate)
{
    //Tilemap *map = LocalPlayerMap();
    //map->UpdateAnimations(client.frameDt);

    if (simulate) {
        UpdatePower(now);
    }

    // TODO: Only run this when the edge list is dirty
    UpdateEdges(edges);
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

    if (entity.type == Entity::TYP_PLAYER) {
        assert(entity.radius);

        entity.on_warp = false;
        entity.on_warp_coord = {};
        for (int y = yMin; y < yMax && !entity.on_warp; y++) {
            for (int x = xMin; x < xMax && !entity.on_warp; x++) {
                const ObjectData *object = GetObjectData(x, y);
                if (object && object->type == OBJ_WARP) {
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

void Tilemap::DrawTile(uint16_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws, Color color)
{
    // TODO: Yikes.. that's a lot of lookups in a tight loop. Memoize some pointers or something man.
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);
    Rectangle texRect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    if (gfx_frame.h > TILE_W) {
        position.y -= gfx_frame.h - TILE_W;
    }

    if (sortedDraws) {
        Vector3 pos = { position.x, position.y, 0 };
        DrawCmd cmd = DrawCmd::Texture(gfx_file.texture, texRect, pos, color);
        sortedDraws->push(cmd);
    } else {
        dlb_DrawTextureRec(gfx_file.texture, texRect, position, color);
    }
}
void Tilemap::Draw(Camera2D &camera, DrawCmdQueue &sortedDraws)
{
    const Vector2 cursorWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    Rectangle cameraRectWorld = GetCameraRectWorld(camera);

    // NOTE(dlb): Give extra padding on all sides to prevent culling 2-tall objects
    int yMin = CLAMP(-1 + floorf(cameraRectWorld.y / TILE_W), 0, height);
    int yMax = CLAMP( 1 + ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    int xMin = CLAMP(-1 + floorf(cameraRectWorld.x / TILE_W), 0, width);
    int xMax = CLAMP( 1 + ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    DrawCmdQueue *queue = 0;
    for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
        // NOTE(dlb): We don't sort ground tiles
        if (layer > 0) {
            queue = &sortedDraws;
        }

        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                uint16_t tile_id = At((TileLayerType)layer, x, y);
                // NOTE(dlb): We only draw void tiles on ground level
                if (tile_id || layer == 0) {
                    DrawTile(tile_id, { (float)x * TILE_W, (float)y * TILE_W }, queue);
                }
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
            if (IsSolid(x, y)) {
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
            uint16_t tile_id = At(TILE_LAYER_GROUND, x, y);
            Vector2 pos = { (float)x * TILE_W + pad, (float)y * TILE_W + pad };
            DrawTextEx(fntSmall, TextFormat("%d", tile_id), pos, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
        }
    }
}
