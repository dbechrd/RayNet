#pragma once
#include "../common/shared_lib.h"

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
    Entity entities[SV_MAX_ENTITIES]{};
    EntityGhost ghosts[SV_MAX_ENTITIES]{};
    Dialog dialogs[CL_MAX_DIALOGS]{};

    Err CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now);
    void Update(GameClient &gameClient);
    void Draw(void);

private:
    void UpdateEntities(GameClient &gameClient);
    void RemoveExpiredDialogs(GameClient &gameClient);
    void DrawDialog(Dialog &dialog, Vector2 bottomCenter);
    void DrawDialogs(void);
};
