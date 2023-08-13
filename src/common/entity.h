#pragma once
#include "common.h"
#include "../common/ring_buffer.h"

struct Msg_S_EntitySnapshot;

struct GhostSnapshot {
    double   serverTime {};

    // Entity
    uint32_t map_id     {};
    Vector2  position   {};

    // Physics
    float    speed      {};
    Vector2  velocity   {};

    // Life
    int      hp_max     {};
    int      hp         {};

    // TODO: Wtf do I do with this shit?
    uint32_t lastProcessedInputCmd {};

    GhostSnapshot(void) {}
    GhostSnapshot(Msg_S_EntitySnapshot &msg);
};
typedef RingBuffer<GhostSnapshot, CL_SNAPSHOT_COUNT> AspectGhost;

struct EntityData {
    data::Entity entity {};
    AspectGhost  ghosts {};
};
