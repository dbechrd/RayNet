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

struct ProtoDb {
    EntityProto npc_lily{};
    EntityProto npc_freye{};
    EntityProto npc_nessa{};
    EntityProto npc_elane{};
    EntityProto npc_chicken{};
    EntityProto itm_poop{};

    void Load(void);
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
    double frameDt{};
    double frameDtSmooth = 60;

    bool showF3Menu = false;

    // TODO(cleanup): Weird debug viz
    Rectangle lastCollisionA{};
    Rectangle lastCollisionB{};

    uint32_t nextEntityId = 1;

    double lastTownfolkSpawnedAt{};
    double lastChickenSpawnedAt{};
    uint32_t eid_bots[1]{};

    ProtoDb protoDb{};

    GameServer(double now) : now(now), frameStart(now) {};

    void OnClientJoin(int clientIdx);
    void OnClientLeave(int clientIdx);

    uint32_t GetPlayerEntityId(uint32_t clientIdx);
    ServerPlayer *FindServerPlayer(uint32_t entity_id, int *client_idx = 0);

    void LoadProtos(void);  // TODO: Move to pack file
    Err Start(void);
    void Update(void);
    void Stop(void);

private:
    Tilemap *FindOrLoadMap(uint32_t map_id);
    Tilemap *FindMap(uint32_t map_id);

    Entity *SpawnEntity(EntityType type);
    Entity *SpawnEntityProto(uint32_t map_id, Vector3 position, EntityProto &proto);
    Entity *SpawnProjectile(uint32_t map_id, Vector3 position, Vector2 direction, Vector3 initial_velocity);
    void WarpEntity(Entity &entity, uint32_t dest_map_id, Vector3 dest_pos);
    void DespawnEntity(uint32_t entityId);
    void DestroyDespawnedEntities(void);

    void TickPlayers(void);
    void TickSpawnTownNPCs(uint32_t map_id);
    void TickSpawnCaveNPCs(uint32_t map_id);
    void TickEntityNPC(Entity &entity, double dt);
    void TickEntityPlayer(Entity &entity, double dt);
    void TickEntityProjectile(Entity &entity, double dt);
    void TickResolveEntityWarpCollisions(Tilemap &map, Entity &entity, double now);
    void Tick(void);

    void SerializeSpawn(uint32_t entityId, Msg_S_EntitySpawn &entitySpawn);
    void SendEntitySpawn(int clientIdx, uint32_t entityId);
    void BroadcastEntitySpawn(uint32_t entityId);

    void SendEntityDespawn(int clientIdx, uint32_t entityId);
    void BroadcastEntityDespawn(uint32_t entityId);

    void SendEntitySay(int clientIdx, uint32_t entityId, uint32_t dialogId, const std::string &title, const std::string &message);
    void BroadcastEntitySay(uint32_t entityId, const std::string &title, const std::string &message);

    void SendTileChunk(int clientIdx, Tilemap &map, uint32_t x, uint32_t y);
    void BroadcastTileChunk(Tilemap &map, uint32_t x, uint32_t y);

    void SendTileUpdate(int clientIdx, Tilemap &map, uint32_t x, uint32_t y);
    void BroadcastTileUpdate(Tilemap &map, uint32_t x, uint32_t y);

    void SendTitleShow(int clientIdx, const std::string &text);

    // All part of Update()
    void RequestDialog(int clientIdx, Entity &entity, Dialog &dialog);

    void ProcessMsg(int clientIdx, Msg_C_EntityInteract &msg);
    void ProcessMsg(int clientIdx, Msg_C_EntityInteractDialogOption &msg);
    void ProcessMsg(int clientIdx, Msg_C_InputCommands &msg);
    void ProcessMsg(int clientIdx, Msg_C_TileInteract &msg);
    void ProcessMessages(void);

    void SerializeSnapshot(Entity &entity, Msg_S_EntitySnapshot &entitySnapshot);
    void SendClientSnapshots(void);
    void SendClockSync(void);
};
