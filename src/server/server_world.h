#pragma once
#include "../common/common.h"
#include "../common/entity.h"
#include "../common/tilemap.h"

struct GameServer;

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
    uint32_t entity_freelist;
    ServerPlayer players[SV_MAX_PLAYERS]{};

    ServerWorld();
    uint32_t GetPlayerEntityId(uint32_t clientIdx);
    uint32_t CreateEntity(EntityType entityType);
    void SpawnEntity(GameServer &server, uint32_t entityId);
    Entity *GetEntity(uint32_t entityId);
    void DespawnEntity(GameServer &server, uint32_t entityId);
    void DestroyEntity(uint32_t entityId);
};
