#include "entity_db.h"
#include <cassert>

// TODO: Move this into GameClient prolly, eh?
EntityDB *entityDb{};

size_t EntityDB::FindEntityIndex(uint32_t entityId)
{
    if (entityId) {
        const auto &entry = entityIndexById.find(entityId);
        if (entry != entityIndexById.end()) {
            size_t entityIndex = entry->second;
            assert(entityIndex < SV_MAX_ENTITIES);
            return entityIndex;
        }
    }
    return 0;
}
data::Entity *EntityDB::FindEntity(uint32_t entityId, bool evenIfDespawned)
{
    size_t entityIndex = FindEntityIndex(entityId);
    if (entityIndex) {
        data::Entity &entity = entities[entityIndex];
        assert(entity.id == entityId);
        assert(entity.type);
        if (evenIfDespawned || !entity.despawned_at) {
            return &entity;
        }
    }
    return 0;
}
data::Entity *EntityDB::FindEntity(uint32_t entityId, data::EntityType type, bool evenIfDespawned)
{
    data::Entity *entity = FindEntity(entityId, evenIfDespawned);
    if (entity && entity->type != type) {
        return 0;
    }
    return entity;
}
data::Entity *EntityDB::SpawnEntity(uint32_t entityId, data::EntityType type, double now)
{
    assert(entityId);
    assert(type);

    data::Entity *entity = FindEntity(entityId);
    if (entity) {
        printf("[entity_db] Failed to spawn entity id %u of type %s. An entity with that id already exists.\n", entityId, EntityTypeStr(type));
        assert(!"entity already exists.. huh?");
        return 0;
    }

    if (entityIndexById.size() < SV_MAX_ENTITIES) {
        for (int i = 1; i < entities.size(); i++) {
            if (!entities[i].id) {
                entity = &entities[i];
                entity->id = entityId;
                entity->type = type;
                entity->spawned_at = now;
                entityIndexById[entityId] = i;
                break;
            }
        }
    }

    if (!entity) {
        printf("[entity_db] Failed to spawn entity id %u of type %s. Max entities.\n", entityId, EntityTypeStr(type));
    }
    return entity;
}
bool EntityDB::DespawnEntity(uint32_t entityId, double now)
{
    assert(entityId);

    data::Entity *entity = FindEntity(entityId);
    if (entity && !entity->despawned_at) {
        entity->despawned_at = now;
        return true;
    } else {
        printf("[entity_db] Failed to despawn entity id %u. Entity id not found.\n", entityId);
    }
    return false;
}
void EntityDB::DestroyEntity(uint32_t entityId)
{
    assert(entityId);

    size_t entityIndex = FindEntityIndex(entityId);
    if (entityIndex) {
        data::Entity &entity = entities[entityIndex];
        assert(entity.id);   // wtf happened man
        assert(entity.type); // wtf happened man

        // Clear aspects
        entities [entityIndex] = {};
        ghosts   [entityIndex] = {};

        // Remove from map
        entityIndexById.erase(entityId);
    } else {
        assert(0);
        printf("error: entityId %u out of range\n", entityId);
    }
}

Rectangle EntityDB::EntityRect(uint32_t entityId)
{
    assert(entityId);

    data::Entity *entity = FindEntity(entityId);
    if (entity) {
        return data::GetSpriteRect(*entity);
    }
    return {};
}
Vector2 EntityDB::EntityTopCenter(uint32_t entityId)
{
    assert(entityId);

    const Rectangle rect = EntityRect(entityId);
    const Vector2 topCenter{
        rect.x + rect.width / 2,
        rect.y
    };
    return topCenter;
}
void EntityDB::EntityTick(data::Entity &entity, double dt)
{
    Vector3 &pos = entity.position;
    Vector3 &vel = entity.velocity;

    vel = Vector3Add(vel, Vector3Scale(entity.force_accum, dt));
    entity.force_accum = {};

#if 0
    const float &drag = data.physics.drag * dt;

    //vel.x = CLAMP(vel.x, -200, 200);
    if (vel.x > 0) {
        vel.x = MAX(0, vel.x - drag);
    } else if (vel.x < 0) {
        vel.x = MIN(vel.x + drag, 0);
    }

    //vel.y = CLAMP(vel.y, -200, 200);
    if (vel.y > 0) {
        vel.y = MAX(0, vel.y - drag);
    } else if (vel.y < 0) {
        vel.y = MIN(vel.y + drag, 0);
    }
#elif 0
    const float drag = (1.0f - data.physics.drag) * dt;
    vel = Vector2Scale(vel, drag);
#elif 0
    const float drag = (1.0f - data.physics.drag * SV_TICK_DT);
    if (dt == SV_TICK_DT) {
        vel = Vector2Scale(vel, drag);
    } else {
        const float alpha = dt / SV_TICK_DT;
        vel = Vector2Scale(vel, drag * alpha);
    }
#else
    // https://www.reddit.com/r/Unity3D/comments/5qla41/comment/dd0jp6o/?utm_source=reddit&utm_medium=web2x&context=3
    // Lerping: current = Mathf.Lerp(current, target, 1.0f - Mathf.Exp(-Sharpness * Time.deltaTime));
    // Damping: current *= Mathf.Exp(-Sharpness * Time.deltaTime);

    vel = Vector3Scale(vel, exp2f(-10.0f * (entity.drag) * dt));
#endif

    pos = Vector3Add(pos, Vector3Scale(vel, dt));
}

void EntityDB::DrawEntityIds(uint32_t entityId, Camera2D &camera)
{
    assert(entityId);
    data::Entity *entity = FindEntity(entityId);
    if (entity) {
        assert(entity->id == entityId);
        assert(entity->type);
        DrawTextEx(fntSmall, TextFormat("%u", entity->id), entity->ScreenPos(),
            fntSmall.baseSize / camera.zoom, 1 / camera.zoom, WHITE);
    }
}
void EntityDB::DrawEntity(data::Entity &entity, data::DrawCmdQueue &sortedDraws, bool highlight)
{
    const Rectangle rect = data::GetSpriteRect(entity);
    data::DrawSprite(entity, &sortedDraws, highlight);
}