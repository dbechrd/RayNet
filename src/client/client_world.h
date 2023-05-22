#pragma once
#include "../common/common.h"
#include "../common/tilemap.h"

struct Msg_S_EntitySpawn;
struct Msg_S_EntitySnapshot;
struct GameClient;

struct GhostSnapshot {
    double   serverTime {};

    // Entity
    Vector2  position   {};

    // Physics
    float    speed      {};
    Vector2  velocity   {};

    // Life
    int      maxHealth  {};
    int      health     {};

    // TODO: Wtf do I do with this shit?
    uint32_t lastProcessedInputCmd {};

    GhostSnapshot(void) {}
    GhostSnapshot(Msg_S_EntitySnapshot &msg);
};

typedef RingBuffer<GhostSnapshot, CL_SNAPSHOT_COUNT> Ghost;

struct Dialog {
    uint32_t entityId;
    double spawnedAt;
    uint32_t messageLength;
    char *message;
};

struct ClientWorld {
    Camera2D camera2d{};
    Tilemap map{};

    // TODO: This should go in Tilemap on client, but server doesn't need it.
    //       Where's the right place for it to go then?
    Ghost ghosts[SV_MAX_ENTITIES]{};
    Dialog dialogs[CL_MAX_DIALOGS]{};

    Entity *GetEntity(uint32_t entityId);
    Entity *GetEntityDeadOrAlive(uint32_t entityId);
    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);
    void ApplyStateInterpolated(uint32_t entityId, const GhostSnapshot &a, const GhostSnapshot &b, float alpha);
    Err CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now);
    Err DestroyDialog(uint32_t dialogId);
    void Update(GameClient &gameClient);
    void Draw(void);

private:
    void UpdateEntities(GameClient &gameClient);
    void RemoveExpiredDialogs(GameClient &gameClient);
    void DrawDialog(Dialog &dialog, Vector2 bottomCenter);
    void DrawDialogs(void);
};
