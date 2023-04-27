#include "server_world.h"

ServerWorld::ServerWorld()
{
    for (uint32_t i = freelist_head; i < SV_MAX_ENTITIES - 1; i++) {
        Entity &entity = entities[i];
        entities[i].freelist_next = i + 1;
    }
}

uint32_t ServerWorld::CreateEntity(EntityType entityType)
{
    uint32_t entityId = freelist_head;
    if (freelist_head) {
        Entity &e = entities[freelist_head];
        freelist_head = e.freelist_next;
        e.freelist_next = 0;
        e.type = entityType;
    }
    return entityId;
}

void ServerWorld::SpawnEntity(uint32_t entityId, double now)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.spawnedAt = now;
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

void ServerWorld::DespawnEntity(uint32_t entityId, double now)
{
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.despawnedAt = now;
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
            entity.freelist_next = freelist_head;
            freelist_head = entityId;
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}