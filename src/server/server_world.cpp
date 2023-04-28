#include "server_world.h"
#include "game_server.h"

ServerWorld::ServerWorld()
{
    entity_freelist = SV_MAX_PLAYERS + 1;  // 0 reserved, 1-MAX_PLAYERS reserved
    for (uint32_t i = entity_freelist; i < SV_MAX_ENTITIES - 1; i++) {
        Entity &entity = entities[i];
        entities[i].freelist_next = i + 1;
    }
}

uint32_t ServerWorld::GetPlayerEntityId(uint32_t clientIdx)
{
    return clientIdx + 1;
}

uint32_t ServerWorld::CreateEntity(EntityType entityType)
{
    uint32_t entityId = entity_freelist;
    if (entity_freelist) {
        Entity &e = entities[entity_freelist];
        entity_freelist = e.freelist_next;
        e.freelist_next = 0;
        e.type = entityType;
    }
    return entityId;
}

void ServerWorld::SpawnEntity(GameServer &server, uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.spawnedAt = server.now;
            server.BroadcastEntitySpawn(entityId);
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}

Entity *ServerWorld::GetEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            return &entity;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
    return 0;
}

void ServerWorld::DespawnEntity(GameServer &server, uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.despawnedAt = server.now;
            server.BroadcastEntityDespawn(entityId);
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}

void ServerWorld::DestroyEntity(uint32_t entityId)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity = {};
            entity.freelist_next = entity_freelist;
            entity_freelist = entityId;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}