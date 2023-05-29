#pragma once
#include "../common/common.h"
#include "../common/tilemap.h"

struct Msg_S_EntitySpawn;
struct Msg_S_EntitySnapshot;
struct GameClient;

struct ClientWorld {
    Camera2D camera2d{};

    uint32_t localPlayerEntityId{};

    std::vector<Tilemap *> maps{};
    std::unordered_map<uint32_t, size_t> mapsById{};     // maps by their map id
    std::unordered_map<uint32_t, size_t> entityMapId{};  // maps by entity id (i.e. which map an entity is currently in)

    ~ClientWorld(void);
    Entity *LocalPlayer(void);
    Tilemap *LocalPlayerMap(void);

    Tilemap *FindOrLoadMap(uint32_t mapId);
    Tilemap *FindEntityMap(uint32_t entityId);
    Entity *FindEntity(uint32_t entityId, bool deadOrAlive = false);

    void ApplySpawnEvent(const Msg_S_EntitySpawn &entitySpawn);
    void DespawnEntity(uint32_t entityId);

    void ApplyStateInterpolated(uint32_t entityId, const GhostSnapshot &a, const GhostSnapshot &b, float alpha);
    Err CreateDialog(uint32_t entityId, uint32_t messageLength, const char *message, double now);
    void Update(GameClient &gameClient);
    void Draw(void);

private:
    void UpdateEntities(GameClient &gameClient);
    void DrawDialog(AspectDialog &dialog, Vector2 topCenter);
    void DrawDialogs(void);
};
