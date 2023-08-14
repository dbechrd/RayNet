#pragma once
#include "../common/common.h"
#include "../common/tilemap.h"

struct Msg_S_EntitySpawn;
struct Msg_S_EntitySnapshot;
struct GameClient;

struct Controller {
    int nextSeq{};  // next input command sequence number to use

    InputCmd cmdAccum{};         // accumulate input until we're ready to sample
    double sampleInputAccum{};   // when this fills up, we are due to sample again
    double lastInputSampleAt{};  // time we last sampled accumulator
    double lastCommandSentAt{};  // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples
};

struct ClientWorld {
    Camera2D camera2d{};
    bool showSnapshotShadows{};

    uint32_t localPlayerEntityId{};
    uint32_t hoveredEntityId{};

    std::vector<Tilemap *> maps{};
    std::unordered_map<uint32_t, size_t> mapsById{};     // maps by their map id

    std::string musBackgroundMusic{};

    ~ClientWorld(void);
    data::Entity *LocalPlayer(void);
    Tilemap *LocalPlayerMap(void);

    Tilemap *FindOrLoadMap(uint32_t mapId);

    bool CopyEntityData(uint32_t entityId, EntityData &data);

    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);

    void ApplyStateInterpolated(data::Entity &entity,
        const GhostSnapshot &a, const GhostSnapshot &b, float alpha, float dt);
    void ApplyStateInterpolated(uint32_t entityId,
        const GhostSnapshot &a, const GhostSnapshot &b, float alpha, float dt);
    Err CreateDialog(uint32_t entityId, uint32_t dialogId, std::string message, double now);
    void Update(GameClient &client);
    void Draw(GameClient &client);

private:
    struct HistoData {
        uint32_t latestSnapInputSeq;
        double cmdAccumDt;
        Vector2 cmdAccumForce;
    };

    void UpdateLocalPlayerHisto(GameClient &client, data::Entity &entity, HistoData &histoData);
    void UpdateLocalPlayer(GameClient &client, data::Entity &entity, AspectGhost &ghost);
    void UpdateLocalGhost(GameClient &client, data::Entity &entity, AspectGhost &ghost, Tilemap *localPlayerMap);
    void UpdateEntities(GameClient &client);

    void DrawEntitySnapshotShadows(uint32_t entityId, Controller &controller, double now, double dt);
    void DrawDialog(GameClient &client, data::Entity &entity, Vector2 bottomCenterScreen);
    void DrawDialogs(GameClient &client, Camera2D &camera);
};
