#pragma once
#include "common.h"
#include "data.h"

struct EntityDB {
    std::unordered_map<uint32_t, size_t> entityIndexById{};

    // TODO: Rename these so they don't collide with local variables all the time
    std::array<data::Entity     , SV_MAX_ENTITIES> entities {};
    std::array<data::AspectGhost, SV_MAX_ENTITIES> ghosts   {};

    size_t FindEntityIndex(uint32_t entityId);
    data::Entity *FindEntity(uint32_t entityId, bool evenIfDespawned = false);
    data::Entity *FindEntity(uint32_t entityId, data::EntityType type, bool evenIfDespawned = false);
    data::Entity *SpawnEntity(uint32_t entityId, data::EntityType type, double now);
    bool DespawnEntity(uint32_t entityId, double now);
    void DestroyEntity(uint32_t entityId);

    Rectangle EntityRect(uint32_t entityId);
    Vector2 EntityTopCenter(uint32_t entityId);
    void EntityTick(data::Entity &entity, double dt);

    void DrawEntityIds(uint32_t map_id, Camera2D &camera);
    void DrawEntity(data::Entity &entity, data::DrawCmdQueue &sortedDraws, bool highlight = false);
};

extern EntityDB *entityDb;