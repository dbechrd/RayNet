#pragma once
#include "../common/common.h"
#include "../common/net/entity_snapshot.h"
#include "../common/tilemap.h"

struct GameClient;

struct EntityGhost {
    RingBuffer<EntitySnapshot, CL_SNAPSHOT_COUNT> snapshots{};
};

struct Dialog {
    uint32_t entityId;
    double spawnedAt;
    uint32_t messageLength;
    char *message;
};

struct ClientWorld {
    Camera2D camera2d{};
    Tilemap map{};
    EntityGhost ghosts[SV_MAX_ENTITIES]{};
    Dialog dialogs[CL_MAX_DIALOGS]{};

    Entity *GetEntity(uint32_t entityId);
    Entity *GetEntityDeadOrAlive(uint32_t entityId);
    Err CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now);
    Err DestroyDialog(uint32_t dialogId);
    void Update(GameClient &gameClient);
    void Draw(void);

private:
    Entity entities[SV_MAX_ENTITIES]{};

    void UpdateEntities(GameClient &gameClient);
    void RemoveExpiredDialogs(GameClient &gameClient);
    void DrawDialog(Dialog &dialog, Vector2 bottomCenter);
    void DrawDialogs(void);
};
