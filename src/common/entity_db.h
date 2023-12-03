#pragma once
#include "common.h"
#include "data.h"

struct EntityDB {
    std::unordered_map<uint32_t, size_t>     entities_by_id {};
    std::array<Entity     , SV_MAX_ENTITIES> entities       {};
    std::array<AspectGhost, SV_MAX_ENTITIES> ghosts         {};

    Entity *FindEntity(uint32_t entity_id, bool evenIfDespawned = false);
    Entity *FindEntity(uint32_t entity_id, EntityType type, bool evenIfDespawned = false);
    Entity *SpawnEntity(uint32_t entity_id, EntityType type, double now);
    bool DespawnEntity(uint32_t entity_id, double now);
    void DestroyEntity(uint32_t entity_id);

    Rectangle EntityRect(uint32_t entity_id);
    Vector2 EntityTopCenter(uint32_t entity_id);
    void EntityTick(Entity &entity, double dt);

    void DrawEntityIds(uint32_t map_id, Camera2D &camera);
    void DrawEntity(Entity &entity, DrawCmdQueue &sortedDraws, bool highlight = false);
};

extern EntityDB *entityDb;