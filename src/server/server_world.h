#pragma once
#include "../common/common.h"
#include "../common/entity.h"
#include "../common/tilemap.h"

struct ServerPlayer {
    //uint32_t clientIdx      {};  // yj_client index
    double   joinedAt       {};
    bool     needsClockSync {};
    uint32_t entityId       {};
    uint32_t lastInputSeq   {};  // sequence number of last input command we processed
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> inputQueue{};
};

struct ServerWorld {
    Camera2D camera2d{};
    Tilemap map{};
    Entity entities[SV_MAX_ENTITIES]{};
    ServerPlayer players[SV_MAX_PLAYERS]{};
    uint32_t freelist_head = SV_MAX_PLAYERS;

    ServerWorld();
    uint32_t CreateEntity(EntityType entityType);
    void SpawnEntity(uint32_t entityId, double now);
    Entity *GetEntity(uint32_t entityId);
    void DespawnEntity(uint32_t entityId, double now);
    void DestroyEntity(uint32_t entityId);
};
