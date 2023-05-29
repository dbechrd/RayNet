#pragma once
#include "../common/input_command.h"
#include "../common/tilemap.h"

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

    uint64_t tick{};
    double tickAccum{};
    double lastTickedAt{};
    double now{};

    Histogram histogram{};
    double frameStart{};
    double frameDt = 0;
    double frameDtSmooth = 60;

    bool showF3Menu = false;

    uint32_t nextMapId = 1;
    uint32_t nextEntityId = 1;
    std::vector<Tilemap *> maps{};
    std::unordered_map<uint32_t, size_t> mapsById{};     // maps by their map id
    std::unordered_map<uint32_t, size_t> entityMapId{};  // maps by entity id (i.e. which map an entity is currently in)

    GameServer(double now) : now(now), frameStart(now) {};
    ~GameServer(void);

    void OnClientJoin(int clientIdx);
    void OnClientLeave(int clientIdx);

    uint32_t GetPlayerEntityId(uint32_t clientIdx);

    Err LoadMap(std::string filename);
    Err Start(void);

    Tilemap *FindMap(uint32_t mapId);
    Tilemap *FindEntityMap(uint32_t entityId);
    Entity *FindEntity(uint32_t entityId);

    uint32_t CreateEntity(Tilemap &map, EntityType type);
    void SpawnEntity(uint32_t entityId);
    void DespawnEntity(uint32_t entityId);

    void BroadcastEntityDespawnTest(uint32_t testId);

    void Update(void);

    void Stop(void);

private:
    void DestroyDespawnedEntities(void);

    void SerializeSpawn(Tilemap &map, size_t entityIndex, Msg_S_EntitySpawn &entitySpawn);
    void SendEntitySpawn(int clientIdx, Tilemap &map, uint32_t entityId);
    void BroadcastEntitySpawn(Tilemap &map, uint32_t entityId);

    void SendEntityDespawn(int clientIdx, Tilemap &map, uint32_t entityId);
    void BroadcastEntityDespawn(Tilemap &map, uint32_t entityId);

    void SendEntityDespawnTest(int clientIdx, uint32_t testId);

    void SendEntitySay(int clientIdx, Tilemap &map, uint32_t entityId, uint32_t messageLength, const char *message);
    void BroadcastEntitySay(Tilemap &map, uint32_t entityId, uint32_t messageLength, const char *message);

    void SendTileChunk(int clientIdx, Tilemap &map, uint32_t x, uint32_t y);
    void BroadcastTileChunk(Tilemap &map, uint32_t x, uint32_t y);

    // All part of Update()
    void ProcessMessages(void);
    uint32_t SpawnProjectile(Tilemap &map, Vector2 position, Vector2 direction);
    void UpdateServerPlayers(void);
    void TickSpawnBots(Tilemap &map);
    void TickEntityBot(Tilemap &map, uint32_t entityIndex, double dt);
    void TickEntityPlayer(Tilemap &map, uint32_t entityIndex, double dt);
    void TickEntityProjectile(Tilemap &map, uint32_t entityIndex, double dt);
    void TickPlayer(Tilemap &map, uint32_t entityId, double dt);
    void TickBot(Tilemap &map, uint32_t entityId, double dt);
    void TickProjectile(Tilemap &map, uint32_t entityId, double dt);
    void Tick(void);
    void SerializeSnapshot(Tilemap &map, size_t entityId, Msg_S_EntitySnapshot &entitySnapshot, uint32_t lastProcessedInputCmd);  // TODO: Remove lastProcessedInputCmd from args list, shouldn't need it
    void SendClientSnapshots(void);
    void SendClockSync(void);
};

extern Rectangle lastCollisionA;
extern Rectangle lastCollisionB;