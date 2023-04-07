#pragma once
#include "../common/common.h"
#include "../common/entity.h"

struct EntityGhost {
    RingBuffer<EntitySnapshot, CL_SNAPSHOT_COUNT> snapshots{};
};

struct ClientWorld {
    Entity entities[SV_MAX_ENTITIES]{};
    EntityGhost ghosts[SV_MAX_ENTITIES]{};
};
