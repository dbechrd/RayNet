#include "server_world.h"

ServerWorld::ServerWorld()
{
    for (uint32_t i = freelist_head; i < SV_MAX_ENTITIES - 1; i++) {
        Entity &entity = entities[i];
        entities[i].freelist_next = i + 1;
    }
}

uint32_t ServerWorld::MakeEntity(EntityType entityType) {
    uint32_t entityId = freelist_head;
    if (freelist_head) {
        Entity &e = entities[freelist_head];
        freelist_head = e.freelist_next;
        e.freelist_next = 0;

        e.type = entityType;
        e.spawnedAt = GetTime();
    }
    return entityId;
}

void ServerWorld::DespawnEntity(uint32_t entityId) {
    if (entityId < SV_MAX_ENTITIES) {
        Entity &entity = entities[entityId];
        if (entity.type) {
            entity.despawnedAt = GetTime();
        }
    } else {
        printf("error: entityId %u out of range\n", entityId);
    }
}

void ServerWorld::DestroyEntity(uint32_t entityId) {
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