#pragma once
#include "server_world.h"

struct ServerWorld;

class GameServerNetAdapter : public NetAdapter
{
public:
    struct GameServer *server;

    GameServerNetAdapter(GameServer *server);
    void OnServerClientConnected(int clientIdx) override;
    void OnServerClientDisconnected(int clientIdx) override;
};

struct GameServer {
    GameServerNetAdapter adapter{ this };
    yojimbo::Server *yj_server{};

    uint64_t tick{};
    double tickAccum{};
    double lastTickedAt{};
    double now{};

    ServerWorld *world{};

    void OnClientJoin(int clientIdx);
    void OnClientLeave(int clientIdx);

    Err Start(void);
    void SendEntitySpawn(int clientIdx, uint32_t entityId);
    void BroadcastEntitySpawn(uint32_t entityId);
    void SendEntityDespawn(int clientIdx, uint32_t entityId);
    void BroadcastEntityDespawn(uint32_t entityId);
    void SendEntitySay(int clientIdx, uint32_t entityId, uint32_t messageLength, const char *message);
    void BroadcastEntitySay(uint32_t entityId, uint32_t messageLength, const char *message);
    void SendTileChunk(int clientIdx, uint32_t x, uint32_t y);
    void BroadcastTileChunk(uint32_t x, uint32_t y);
    void ProcessMessages(void);
    void Tick(void);
    void SendClientSnapshots(void);
    void DestroyDespawnedEntities(void);
    void SendClockSync(void);
    void Update(void);
    void Stop(void);
};

extern Rectangle lastCollisionA;
extern Rectangle lastCollisionB;