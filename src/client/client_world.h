#pragma once
#include "../common/common.h"
#include "../common/entity.h"
#include "../common/tilemap.h"

struct EntityGhost {
    RingBuffer<EntitySnapshot, CL_SNAPSHOT_COUNT> snapshots{};
};

struct ClientWorld {
    Camera2D camera2d{};
    Tilemap map{};
    Entity entities[SV_MAX_ENTITIES]{};
    EntityGhost ghosts[SV_MAX_ENTITIES]{};
};
