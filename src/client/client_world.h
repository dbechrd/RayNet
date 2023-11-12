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
    Camera2D camera{};
    double fadeDirection{};
    double fadeDuration{};
    double fadeValue{};

    uint32_t last_target_id = 0;
    std::string last_target_map = "";
    float zoomTarget = camera.zoom;

    bool showSnapshotShadows{};

    uint32_t localPlayerEntityId{};
    uint32_t hoveredEntityId{};   // mouse is over entity
    bool hoveredEntityInRange{};  // player is close enough to interact

    const char *hudSpinnerItems[2]{
        "Fireball",
        "Shovel"
    };

    bool hudSpinnerPrev = false;
    bool hudSpinner = false;
    Vector2 hudSpinnerPos{};
    int hudSpinnerIndex = 0;  // which index is currently active
    int hudSpinnerCount = 6;  // how many items in hud spinner

    inline const char *HudSpinnerItemName(void) {
        const char *holdingItem = hudSpinnerIndex < ARRAY_SIZE(hudSpinnerItems) ? hudSpinnerItems[hudSpinnerIndex] : 0;
        return holdingItem;
    }

    // For histogram
    float prevX = 0;

    ClientWorld(void);

    data::Entity *LocalPlayer(void);
    data::Tilemap *LocalPlayerMap(void);
    data::Tilemap *FindOrLoadMap(const std::string &map_id);

    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);
    void ApplyStateInterpolated(data::Entity &entity, const data::GhostSnapshot &a, const data::GhostSnapshot &b, float alpha, float dt);
    Err CreateDialog(data::Entity &entity, uint32_t dialogId, const std::string &title, const std::string &message, double now);

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
    void UpdateCamera(GameClient &client);
    void UpdateHUDSpinner(void);

    void DrawEntitySnapshotShadows(GameClient &client, data::Entity &entity, Controller &controller);
    void DrawEntities(GameClient &client, data::Tilemap &map, data::DrawCmdQueue &sortedDraws);
    void DrawDialog(GameClient &client, data::Entity &entity, Vector2 bottomCenterScreen, std::vector<FancyTextTip> &tips);
    void DrawDialogTips(std::vector<FancyTextTip> tips);
    void DrawDialogs(GameClient &client);
    void DrawHUDEntityHoverInfo(void);
    void DrawHUDSpinner(void);
    void DrawHUDSignEditor(void);
    void DrawHUDMenu(void);
};
