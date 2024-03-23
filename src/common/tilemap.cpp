#include "collision.h"
#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "wang.h"
#include "flood_fill.h"

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

void Tilemap::Autotile(TileLayerType layer, uint16_t x, uint16_t y, double now)
{
    enum AutoDir { NW, N, NE, W, E, SW, S, SE };

    uint16_t center_tile_id = 0;
    if (!AtTry(layer, x, y, center_tile_id)) {
        return;
    }
    const TileDef &center_def = GetTileDef(center_tile_id);
    if (!center_def.auto_tile_group) {
        return;
    }

    uint16_t tile_ids[8]{};
    AtTry(layer, x - 1, y - 1, tile_ids[0]);
    AtTry(layer, x    , y - 1, tile_ids[1]);
    AtTry(layer, x + 1, y - 1, tile_ids[2]);
    AtTry(layer, x - 1, y    , tile_ids[3]);
    AtTry(layer, x + 1, y    , tile_ids[4]);
    AtTry(layer, x - 1, y + 1, tile_ids[5]);
    AtTry(layer, x    , y + 1, tile_ids[6]);
    AtTry(layer, x + 1, y + 1, tile_ids[7]);

    const TileDef *tile_defs[8]{};
    tile_defs[0] = &GetTileDef(tile_ids[0]);
    tile_defs[1] = &GetTileDef(tile_ids[1]);
    tile_defs[2] = &GetTileDef(tile_ids[2]);
    tile_defs[3] = &GetTileDef(tile_ids[3]);
    tile_defs[4] = &GetTileDef(tile_ids[4]);
    tile_defs[5] = &GetTileDef(tile_ids[5]);
    tile_defs[6] = &GetTileDef(tile_ids[6]);
    tile_defs[7] = &GetTileDef(tile_ids[7]);

    bool match[8]{
        tile_defs[0]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[1]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[2]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[3]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[4]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[5]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[6]->auto_tile_group == center_def.auto_tile_group,
        tile_defs[7]->auto_tile_group == center_def.auto_tile_group,
    };

    int auto_masks[8]{
        0b10000000,  // top left
        0b01000000,  // top
        0b00100000,  // top right
        0b00010000,  // left
        0b00001000,  // right
        0b00000100,  // bottom left
        0b00000010,  // bottom
        0b00000001,  // bottom right
    };

    int center_mask = 0;
    if (match[0]) center_mask |= auto_masks[0];
    if (match[1]) center_mask |= auto_masks[1];
    if (match[2]) center_mask |= auto_masks[2];
    if (match[3]) center_mask |= auto_masks[3];
    if (match[4]) center_mask |= auto_masks[4];
    if (match[5]) center_mask |= auto_masks[5];
    if (match[6]) center_mask |= auto_masks[6];
    if (match[7]) center_mask |= auto_masks[7];

    TileDef *new_tile = FindTileDefByMask(center_def.auto_tile_group, center_mask);
    if (new_tile) {
        Set(layer, x, y, new_tile->id, now, false);
        return;
    }

    // Check for a partial match by ignoring corners that aren't fully connected (via both adjacent edges)
    if (!(match[N] && match[W])) center_mask &= ~auto_masks[NW];
    if (!(match[N] && match[E])) center_mask &= ~auto_masks[NE];
    if (!(match[S] && match[W])) center_mask &= ~auto_masks[SW];
    if (!(match[S] && match[E])) center_mask &= ~auto_masks[SE];

    new_tile = FindTileDefByMask(center_def.auto_tile_group, center_mask);
    if (new_tile) {
        Set(layer, x, y, new_tile->id, now, false);
        return;
    }

    // Check for a partial by ignoring all corners
    new_tile = FindTileDefByMask(center_def.auto_tile_group, center_mask & ~auto_masks[NW] & ~auto_masks[NE] & ~auto_masks[SW] & ~auto_masks[SE]);
    if (new_tile) {
        Set(layer, x, y, new_tile->id, now, false);
        return;
    }
}
void Tilemap::Set(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now, bool autotile)
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

    if (autotile) {
        Autotile(layer, x, y, now);

        Autotile(layer, x - 1, y - 1, now);
        Autotile(layer, x    , y - 1, now);
        Autotile(layer, x + 1, y - 1, now);
        Autotile(layer, x - 1, y    , now);
        Autotile(layer, x + 1, y    , now);
        Autotile(layer, x - 1, y + 1, now);
        Autotile(layer, x    , y + 1, now);
        Autotile(layer, x + 1, y + 1, now);
    }
}
bool Tilemap::SetTry(TileLayerType layer, uint16_t x, uint16_t y, uint16_t tile_id, double now, bool autotile)
{
    if (x >= 0 && y >= 0 && x < width && y < height) {
        Set(layer, x, y, tile_id, now, autotile);
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

struct TileFloodData {
    Tilemap *map;
    TileLayerType layer;
    double now;
};

bool TileFlood_TryGet(int x, int y, void *userdata, int *value)
{
    TileFloodData *data = (TileFloodData *)userdata;
    uint16_t tile_id = 0;
    if (data->map->AtTry(data->layer, x, y, tile_id)) {
        *value = tile_id;
        return true;
    }
    return false;
}

void TileFlood_Set(int x, int y, int value, void *userdata)
{
    TileFloodData *data = (TileFloodData *)userdata;
    data->map->Set(data->layer, x, y, value, data->now);
}

void Tilemap::Flood(TileLayerType layer, uint16_t x, uint16_t y, uint16_t new_tile_id, double now)
{
    TileFloodData data{};
    data.map = this;
    data.layer = layer;
    data.now = now;
    FloodFill filler{ TileFlood_TryGet, TileFlood_Set, (void *)&data };
    filler.Fill(x, y, new_tile_id);
}

bool TileFloodDebug_TryGet(int x, int y, void *userdata, int *value)
{
    Tilemap::TileFloodDebugData *data = (Tilemap::TileFloodDebugData *)userdata;
    uint16_t tile_id = 0;
    if (x >= 0 && x < data->map_w && y >= 0 && y < data->map_h) {
        *value = data->tile_ids_after[y * data->map_w + x];
        return true;
    }
    return false;
}

void TileFloodDebug_Set(int x, int y, int value, void *userdata)
{
    Tilemap::TileFloodDebugData *data = (Tilemap::TileFloodDebugData *)userdata;
    data->tile_ids_after[y * data->map_w + x] = value;
    data->change_list.push_back({ x, y });
}

Tilemap::TileFloodDebugData Tilemap::FloodDebug(TileLayerType layer, uint16_t x, uint16_t y, uint16_t new_tile_id)
{
    TileFloodDebugData data{};
    data.map_w = width;
    data.map_h = height;
    data.tile_ids_before = layers[layer];
    data.tile_ids_after = layers[layer];
    FloodFill filler{ TileFloodDebug_TryGet, TileFloodDebug_Set, (void *)&data };
    filler.Fill(x, y, new_tile_id);

    return data;
}

ObjectData *Tilemap::GetObjectData(uint16_t x, uint16_t y)
{
#if 0
    for (ObjectData &obj_data : object_data) {
        if (obj_data.x == x && obj_data.y == y) {
            return &obj_data;
        }
    }
#else
    const auto &iter = obj_by_coord.find({ x, y });
    if (iter != obj_by_coord.end()) {
        return &object_data[iter->second];
    }
#endif
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
        if (!IsPowerLoad(obj)) {
            continue;
        }

        const int powered = (int)active_power_channels.contains(obj.power_channel);
        if (obj.power_level != powered) {
            obj.power_level = powered;
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
void Tilemap::UpdateEdges(void)
{
    //PerfTimer t{ "UpdateEdges" };
    edges.clear();

    // Clockwise winding, Edge Normal = (-y, x)

    for (int y = 0; y <= height; y++) {
        int topStartIdx = -1;
        int bottomStartIdx = -1;

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
    
    for (int x = 0; x <= width; x++) {
        int leftStartIdx = -1;
        int rightStartIdx = -1;

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
void Tilemap::UpdateIntervals(void)
{
    intervals.clear();

    // TODO(cleanup): UpdatePathfinding() somewhere
}
void Tilemap::Update(double now, bool simulate)
{
    //Tilemap *map = LocalPlayerMap();
    //map->UpdateAnimations(client.frameDt);

    if (simulate) {
        UpdatePower(now);
    }

    // TODO: Only run this when the edge list is dirty
    UpdateEdges();
    UpdateIntervals();
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
            if (Vector2DotProduct(manifold.normal, edge.normal) > 0) {
                Collision collision{};
                collision.edge = &edge;
                collision.manifold = manifold;
                collisions.push_back(collision);
            }
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

void Tilemap::Draw(Camera2D &camera, DrawCmdQueue &sortedDraws)
{
    const Vector2 cursorWorld = GetScreenToWorld2D(GetMousePosition(), camera);

    Rectangle cameraRectWorld = GetCameraRectWorld(camera);

    // NOTE(dlb): Give extra padding on all sides to prevent culling 2-tall objects
    uint16_t yMin = CLAMP(-1 + floorf(cameraRectWorld.y / TILE_W), 0, height);
    uint16_t yMax = CLAMP( 1 + ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    uint16_t xMin = CLAMP(-1 + floorf(cameraRectWorld.x / TILE_W), 0, width);
    uint16_t xMax = CLAMP( 1 + ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

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
    uint16_t yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    uint16_t yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    uint16_t xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    uint16_t xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

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
        edge.Draw(3.0f);
    }
}

bool Tilemap_AnyaSolidQuery(int x, int y, void *userdata)
{
    Tilemap *map = (Tilemap *)userdata;
    return map->IsSolid(x, y);
}
void Tilemap::DrawIntervals(Camera2D &camera)
{
    static Vector2 start{ 25, 45 };

#if 0
    Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
    Coord coord{};
    if (!WorldToTileIndex(world.x, world.y, coord)) {
        return;
    }
    static Vector2 target{ (float)coord.x, (float)coord.y };
#else
    static Vector2 target{ 29, 42 };
#endif
    static bool showGeneratedNodes = true;

    if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (io.KeyDown(KEY_ONE)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
            Coord coord{};
            WorldToTileIndex(mouseWorld.x, mouseWorld.y, coord);
            start = { (float)coord.x, (float)coord.y };
        } else if (io.KeyDown(KEY_TWO)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
            Coord coord{};
            WorldToTileIndex(mouseWorld.x, mouseWorld.y, coord);
            target = { (float)coord.x, (float)coord.y };
        }
    }
    if (io.KeyPressed(KEY_THREE)) {
        showGeneratedNodes = !showGeneratedNodes;
    }

    const char *nodeMode = showGeneratedNodes ? "generate order" : "search order";
    dlb_DrawTextShadowEx(fntMedium, CSTRLEN(nodeMode), GetScreenToWorld2D({ 800, 800 }, camera), WHITE);

    int nodeTint = 0;
    Color greens[] = { DARKGREEN, GREEN, LIME };
    Color blues[] = { DARKBLUE, BLUE, SKYBLUE };

    Anya_State state{ start, target, Tilemap_AnyaSolidQuery, this };
    Anya(state);

    Rectangle startRec{ start.x * TILE_W, start.y * TILE_W, TILE_W, TILE_W };
    DrawRectangleRec(startRec, Fade(LIME, 0.5f));
    DrawRectangleLinesEx(startRec, 3, GREEN);

    Rectangle targetRec{ target.x * TILE_W, target.y * TILE_W, TILE_W, TILE_W };
    DrawRectangleRec(targetRec, Fade(SKYBLUE, 0.5f));
    DrawRectangleLinesEx(targetRec, 3, BLUE);

    const auto &nodes = showGeneratedNodes ? state.nodes : state.nodeSearchOrder;

    if (state.path.size()) {
        DrawLineStrip(state.path.data(), state.path.size(), PINK);
    }

    static int nodeIdx = 0;

    if (io.KeyPressed(KEY_LEFT, true)) {
        nodeIdx--;
    }
    if (io.KeyPressed(KEY_RIGHT, true)) {
        nodeIdx++;
    }
    nodeIdx = CLAMP(nodeIdx, 0, nodes.size() - 1);

#if 0
    // 7.07, 6.40, 5.83, 5.39, 5.10, 5.00, 5.10, 5.39, 5.83, 6.40, 7.07
    // 6.40, 5.66, 5.00, 4.47, 4.12, 4.00, 4.12, 4.47, 5.00, 5.66, 6.40
    // 5.83, 5.00, 4.24, 3.61, 3.16, 3.00, 3.16, 3.61, 4.24, 5.00, 5.83
    // 5.39, 4.47, 3.61, 2.83, 2.24, 2.00, 2.24, 2.83, 3.61, 4.47, 5.39
    // 5.10, 4.12, 3.16, 2.24, 1.41, 1.00, 1.41, 2.24, 3.16, 4.12, 5.10
    // 5.00, 4.00, 3.00, 2.00, 1.00, 0.00, 1.00, 2.00, 3.00, 4.00, 5.00
    // 5.10, 4.12, 3.16, 2.24, 1.41, 1.00, 1.41, 2.24, 3.16, 4.12, 5.10
    // 5.39, 4.47, 3.61, 2.83, 2.24, 2.00, 2.24, 2.83, 3.61, 4.47, 5.39
    // 5.83, 5.00, 4.24, 3.61, 3.16, 3.00, 3.16, 3.61, 4.24, 5.00, 5.83
    // 6.40, 5.66, 5.00, 4.47, 4.12, 4.00, 4.12, 4.47, 5.00, 5.66, 6.40
    // 7.07, 6.40, 5.83, 5.39, 5.10, 5.00, 5.10, 5.39, 5.83, 6.40, 7.07
    for (float y = 0; y <= 10; y++) {
        for (float x = -10; x <= 0; x++) {
            float dist = Vector2Distance({ x, y }, {});
            printf("%.2f ", dist);
        }
        printf("\n");
    }
#endif

    rlDisableBackfaceCulling();

    for (int i = 0; i <= nodeIdx && i < nodes.size(); i++) {
        const auto &node = nodes[i];
        if (i == 0) {
            DrawCircleV(Vector2Scale(node.state->start, TILE_W), 6, RED);
            continue;
        }

        Vector2 r = Vector2Scale(node.root, TILE_W);
        Vector2 p0{ node.interval.x_min * TILE_W, node.interval.y * TILE_W };
        Vector2 p1{ node.interval.x_max * TILE_W, node.interval.y * TILE_W };

#if 0
        // HACK: Make 0-length intervals show up on the screen (this should only affect the start node)
        if (p1.x == p0.x) p1.x += 1;
#endif

        Color color = node.dbgColor;
        if (!color.a) color = YELLOW;

        if (node.interval.y != node.root.y) {
            // cone node
            DrawTriangle(r, p0, p1, Fade(color, i == nodeIdx ? 0.4f : 0.2f));
            DrawTriangleLines(r, p0, p1, color);
        }
            
        DrawLineEx(p0, p1, 4, i == nodeIdx ? color : greens[nodeTint]);

        if (i == nodeIdx) {
            DrawCircleV(Vector2Scale(node.root, TILE_W), 6, RED);
            DrawCircleV(Vector2Scale(node.ClosestPointToTarget(), TILE_W), 4, SKYBLUE);

            Vector2 p = Vector2Subtract(p1, p0);
            Vector2 pHalf = Vector2Add(p0, Vector2Scale(p, 0.5f));
            const char *txt = TextFormat("id = %d, cost = %.2f, depth = %d", node.id, node.cost, node.depth);
            Vector2 txtSize = dlb_MeasureTextShadowEx(fntMedium, CSTRLEN(txt));
            Vector2 txtOffset{ -txtSize.x * 0.5f, -txtSize.y };
            dlb_DrawTextShadowEx(fntMedium, CSTRLEN(txt), Vector2Add(pHalf, txtOffset), WHITE);
        }

        nodeTint++;
        nodeTint %= 3;
    }

#if 0
    auto nodes = Anya_GenStartSuccessors(state);
    for (auto &node : nodes) {
        const auto &i = node.interval;

        DrawLineEx(
            { i.x_min * TILE_W, i.y * TILE_W },
            { i.x_max * TILE_W, i.y * TILE_W },
            8, greens[nodeTint]
        );
        nodeTint++;
        nodeTint %= 3;
    }

    for (auto &node : nodes) {
        auto successors = Anya_Successors(state, node);
        for (auto &successor : successors) {
            const auto &i = successor.interval;
            
            DrawLineEx(
                { i.x_min * TILE_W, i.y * TILE_W },
                { i.x_max * TILE_W, i.y * TILE_W },
                4, blues[nodeTint]
            );
        }
        nodeTint++;
        nodeTint %= 3;
    }
#endif
}
void Tilemap::DrawTileIds(Camera2D &camera, TileLayerType layer)
{
    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    uint16_t yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    uint16_t yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    uint16_t xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    uint16_t xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    const int pad = 8;
    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            uint16_t tile_id = At(layer, x, y);
            Vector2 pos = { (float)x * TILE_W + pad, (float)y * TILE_W + pad };
            DrawTextEx(fntSmall, TextFormat("%d", tile_id), pos, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
        }
    }
}
void Tilemap::DrawObjects(Camera2D &camera)
{
    Rectangle cameraRectWorld = GetCameraRectWorld(camera);
    uint16_t yMin = CLAMP(floorf(cameraRectWorld.y / TILE_W), 0, height);
    uint16_t yMax = CLAMP(ceilf((cameraRectWorld.y + cameraRectWorld.height) / TILE_W), 0, height);
    uint16_t xMin = CLAMP(floorf(cameraRectWorld.x / TILE_W), 0, width);
    uint16_t xMax = CLAMP(ceilf((cameraRectWorld.x + cameraRectWorld.width) / TILE_W), 0, width);

    const Color rect_col = ORANGE;

    for (auto &obj : object_data) {
        if (obj.x >= xMin && obj.x <= xMax && obj.y >= yMin && obj.y <= yMax) {
            Vector2 tilePos = { (float)obj.x * TILE_W, (float)obj.y * TILE_W };
            const int rect_pad = 1;
            Rectangle rect{
                tilePos.x + rect_pad,
                tilePos.y + rect_pad,
                TILE_W - rect_pad * 2,
                TILE_W - rect_pad * 2,
            };
            DrawRectangleRec(rect, Fade(rect_col, 0.6f));
            DrawRectangleLinesEx(rect, 2.0f, rect_col);

            const int text_pad = 8;
            Vector2 pos = { (float)obj.x * TILE_W + text_pad, (float)obj.y * TILE_W + text_pad };
            DrawTextEx(fntSmall, ObjTypeStr(obj.type), { pos.x + 1 / camera.zoom, pos.y + 1 / camera.zoom }, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, BLACK);
            DrawTextEx(fntSmall, ObjTypeStr(obj.type), pos, fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
        }
    }
}

#if _DEBUG
void Tilemap::DrawFloodDebug(TileFloodDebugData &floodDebugData)
{
    for (int i = 0; i < floodDebugData.change_list.size(); i++) {
        const auto &change = floodDebugData.change_list[i];

        Vector2 tilePos = { (float)change.x * TILE_W, (float)change.y * TILE_W };
        const int rect_pad = 1;
        Rectangle rect{
            tilePos.x + rect_pad,
            tilePos.y + rect_pad,
            TILE_W - rect_pad * 2,
            TILE_W - rect_pad * 2,
        };

        const Color rect_col = i <= floodDebugData.step ? PINK : WHITE;
        DrawRectangleRec(rect, Fade(rect_col, 0.6f));
        DrawRectangleLinesEx(rect, 2.0f, rect_col);
    }
}
#endif