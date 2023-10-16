#pragma once
#include "../common/common.h"
#include "../common/data.h"
#include "../common/input_command.h"

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

    std::string musBackgroundMusic{};

    data::Entity *LocalPlayer(void);
    data::Tilemap *LocalPlayerMap(void);

    data::Tilemap *FindOrLoadMap(std::string map_id);

    bool CopyEntityData(uint32_t entityId, data::EntityData &data);

    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);

    void ApplyStateInterpolated(data::Entity &entity,
        const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt);
    void ApplyStateInterpolated(uint32_t entityId,
        const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt);
    Err CreateDialog(uint32_t entityId, uint32_t dialogId, std::string message, double now);
    void Update(GameClient &client);
    void Draw(GameClient &client);

private:
    struct HistoData {
        uint32_t latestSnapInputSeq;
        double cmdAccumDt;
        Vector3 cmdAccumForce;
    };

    void UpdateMap(GameClient &client);
    void UpdateLocalPlayerHisto(GameClient &client, data::Entity &entity, HistoData &histoData);
    void UpdateLocalPlayer(GameClient &client, data::Entity &entity, data::AspectGhost &ghost);
    void UpdateLocalGhost(GameClient &client, data::Entity &entity, data::AspectGhost &ghost, data::Tilemap *localPlayerMap);
    void UpdateEntities(GameClient &client);

    void DrawEntitySnapshotShadows(uint32_t entityId, Controller &controller, double now, double dt);
    void DrawDialogTips(std::vector<FancyTextTip> tips);
    void DrawDialog(GameClient &client, data::Entity &entity, Vector2 bottomCenterScreen, std::vector<FancyTextTip> &tips);
    void DrawDialogs(GameClient &client, Camera2D &camera);
};
