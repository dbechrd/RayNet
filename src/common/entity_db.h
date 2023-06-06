#pragma once
#include "entity.h"

struct EntityDB {
    uint32_t entity_freelist{};
    std::unordered_map<uint32_t, size_t> entityIndexById{};

    // TODO: Rename these so they don't collide with local variables all the time
    std::vector<Entity>          entities  {SV_MAX_ENTITIES};
    std::vector<AspectCollision> collision {SV_MAX_ENTITIES};
    std::vector<AspectDialog>    dialog    {SV_MAX_ENTITIES};
    std::vector<AspectGhost>     ghosts    {SV_MAX_ENTITIES};
    std::vector<AspectLife>      life      {SV_MAX_ENTITIES};
    std::vector<AspectPathfind>  pathfind  {SV_MAX_ENTITIES};
    std::vector<AspectPhysics>   physics   {SV_MAX_ENTITIES};
    std::vector<data::Sprite>    sprite    {SV_MAX_ENTITIES};

    EntityDB(void);

    size_t FindEntityIndex(uint32_t entityId);
    Entity *FindEntity(uint32_t entityId, bool deadOrAlive = false);
    bool SpawnEntity(uint32_t entityId, EntityType entityType, double now);
    bool DespawnEntity(uint32_t entityId, double now);
    void DestroyEntity(uint32_t entityId);

    Rectangle EntityRect(EntitySpriteTuple &data);
    Rectangle EntityRect(uint32_t entityId);
    Vector2 EntityTopCenter(uint32_t entityId);
    void EntityTick(EntityTickTuple &data, double dt);
    void EntityTick(uint32_t entityId, double dt);

    void DrawEntityIds(uint32_t mapId, Camera2D &camera);
    void DrawEntityHoverInfo(uint32_t entityId);
    void DrawEntity(uint32_t entityId);
};

extern EntityDB *entityDb;