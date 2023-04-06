#pragma once
#include "../common/common.h"
#include "../common/entity.h"

struct EntityGhost {
    uint32_t entityId{};
    RingBuffer<EntitySnapshot, CL_SNAPSHOT_COUNT> snapshots{};
};

struct ClientWorld {
    Entity entities[SV_MAX_ENTITIES]{};
    EntityGhost ghosts[SV_MAX_ENTITIES]{};
};
