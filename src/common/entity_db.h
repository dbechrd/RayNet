#pragma once
#include "common.h"
#include "entity.h"

struct EntityDB {
    uint32_t entity_freelist{};
    std::unordered_map<uint32_t, size_t> entityIndexById{};

    // TODO: Rename these so they don't collide with local variables all the time
    std::vector<data::Entity>          entities  {SV_MAX_ENTITIES};
    std::vector<AspectGhost>           ghosts    {SV_MAX_ENTITIES};

    EntityDB(void);

    size_t FindEntityIndex(uint32_t entityId);
    data::Entity *FindEntity(uint32_t entityId, bool evenIfDespawned = false);
    data::Entity *FindEntity(uint32_t entityId, data::EntityType type, bool evenIfDespawned = false);
    data::Entity *SpawnEntity(uint32_t entityId, data::EntityType type, double now);
    bool DespawnEntity(uint32_t entityId, double now);
    void DestroyEntity(uint32_t entityId);

    Rectangle EntityRect(data::Entity &entity);
    Rectangle EntityRect(uint32_t entityId);
    Vector2 EntityTopCenter(uint32_t entityId);
    void EntityTick(data::Entity &entity, double dt);
    void EntityTick(uint32_t entityId, double dt);

    void DrawEntityIds(uint32_t mapId, Camera2D &camera);
    void DrawEntityHoverInfo(uint32_t entityId);
    void DrawEntity(uint32_t entityId);
};

extern EntityDB *entityDb;