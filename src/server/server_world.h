#pragma once
#include "../common/entity.h"
#include "../common/input_command.h"
#include "../common/tilemap.h"

struct GameServer;

struct TileChunkRecord {
    Tilemap::Coord coord{};
    double lastSentAt{};  // when we last sent this chunk to the client
};

struct ServerPlayer {
    //uint32_t clientIdx      {};  // yj_client index
    double   joinedAt       {};
    bool     needsClockSync {};
    uint32_t entityId       {};
    uint32_t lastInputSeq   {};  // sequence number of last input command we processed
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> inputQueue{};
    // TODO(dlb): Also send tile chunks whenever a client enters the render distance of it
    RingBuffer<TileChunkRecord, CL_RENDER_DISTANCE*CL_RENDER_DISTANCE> chunkList{};
};

struct ServerWorld {
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
