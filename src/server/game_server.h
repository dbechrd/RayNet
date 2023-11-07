#pragma once
#include "../common/data.h"
#include "../common/dlg.h"
#include "../common/entity_db.h"
#include "../common/input_command.h"
#include "../common/net/net.h"

// Q: when the player goes to a new level, they see all the wrong entities
// A: entities needs to be scoped by level

// Q: in the editor, you're implicitly editing the actual, live game map, which
//    means i can't edit things like herringbone tiles using my nice editor tools
// A: the editor needs to be able to be editing things (maps?) other than the
//    live game world, but it would be cool to still have that work.

// Q: i want to have multiple kinds of entities, each with unique properties,
//    but I don't want a million branches in my code for updating and
//    serializing every distinct type of object
// A: i need some global "entities" array, at least per-level, with a unique
//    entity id that allows me to identity enties clearly and consistently
//    locally and across the network (and in save files?)

// Q: MSVC keeps whining about some dumb union initialize deleted constructor
//    bullshit that wouldn't happen if I just used C instead of C++.
// A:
//    - Fix the error. Somehow. How does C++ work? Nobody knows. Except ca2.
//    - Remove the union? This is the only choice?
//      - Allocate giant fixed-size buffer like: custom_data[entity_custom_data_size]
//      - Each entity type has e.g. custom_data_projectile that we stick in the buffer
//    - Ignore the error? Definitely not.
//    - Use C instead of C++? No.
//    - Use std::variant. No. Fuck that.

struct Msg_S_EntitySpawn;

struct TileChunkRecord {
    data::Tilemap::Coord coord{};
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
    ServerPlayer players[SV_MAX_PLAYERS]{};  // note: playerIdx = entityId - 1

    uint64_t frame{};
    uint64_t tick{};
    double now{};
    double tickAccum{};
    double lastTickedAt{};

    double frameStart{};
    double frameDt = 0;
    double frameDtSmooth = 60;

    bool showF3Menu = false;

    uint32_t nextEntityId = 1;

    GameServer(double now) : now(now), frameStart(now) {};

    void OnClientJoin(int clientIdx);
    void OnClientLeave(int clientIdx);

    uint32_t GetPlayerEntityId(uint32_t clientIdx);

    Err Start(void);
    void Update(void);
    void Stop(void);

    data::Tilemap *FindOrLoadMap(const std::string &map_id);
    data::Tilemap *FindMap(const std::string &map_id);

    data::Entity *SpawnEntity(data::EntityType type);
    void DespawnEntity(uint32_t entityId);

private:
    void DestroyDespawnedEntities(void);

    void SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn);
    void SendEntitySpawn(int clientIdx, uint32_t entityId);
    void BroadcastEntitySpawn(uint32_t entityId);

    void SendEntityDespawn(int clientIdx, uint32_t entityId);
    void BroadcastEntityDespawn(uint32_t entityId);

    void SendEntityDespawnTest(int clientIdx, uint32_t testId);

    void SendEntitySay(int clientIdx, uint32_t entityId, uint32_t dialogId, const std::string &title, const std::string &message);
    void BroadcastEntitySay(uint32_t entityId, const std::string &title, const std::string &message);

    void SendTileChunk(int clientIdx, data::Tilemap &map, uint32_t x, uint32_t y);
    void BroadcastTileChunk(data::Tilemap &map, uint32_t x, uint32_t y);

    // All part of Update()
    void RequestDialog(int clientIdx, data::Entity &entity, Dialog &dialog);
    void ProcessMessages(void);
    data::Entity *SpawnProjectile(const std::string &map_id, Vector3 position, Vector2 direction, Vector3 initial_velocity);
    void UpdateServerPlayers(void);
    void TickSpawnTownNPCs(const std::string &map_id);
    void TickSpawnCaveNPCs(const std::string &map_id);
    void TickEntityNPC(uint32_t entityIndex, double dt);
    void TickEntityPlayer(uint32_t entityIndex, double dt);
    void TickEntityProjectile(uint32_t entityIndex, double dt);
    void WarpEntity(uint32_t entityId, const std::string &dest_map_id, Vector3 dest_pos);
    void TickResolveEntityWarpCollisions(data::Tilemap &map, uint32_t entityId, double now);
    void Tick(void);
    void SerializeSnapshot(uint32_t entityId, Msg_S_EntitySnapshot &entitySnapshot);
    void SendClientSnapshots(void);
    void SendClockSync(void);
};

extern Rectangle lastCollisionA;
extern Rectangle lastCollisionB;