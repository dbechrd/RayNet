#include "entity_db.h"
#include <cassert>

// TODO: Move this into GameClient prolly, eh?
EntityDB *entityDb{};

Entity *EntityDB::FindEntity(uint32_t entity_id, bool evenIfDespawned)
{
    const auto &iter = entities_by_id.find(entity_id);
    if (iter == entities_by_id.end()) {
        return 0;
    }

    Entity &entity = entities[iter->second];
    assert(entity.type);
    if (entity.despawned_at && !evenIfDespawned) {
        return 0;
    }

    return &entity;
}
Entity *EntityDB::FindEntity(uint32_t entity_id, EntityType type, bool evenIfDespawned)
{
    Entity *entity = FindEntity(entity_id, evenIfDespawned);
    if (entity && entity->type != type) {
        return 0;
    }
    return entity;
}
Entity *EntityDB::SpawnEntity(uint32_t entity_id, EntityType type, double now)
{
    assert(entity_id);
    assert(type);

    Entity *entity = FindEntity(entity_id);
    if (entity) {
        printf("[entity_db] Failed to spawn entity id %u of type %s. An entity with that id already exists.\n", entity_id, EntityTypeStr(type));
        assert(!"entity already exists.. huh?");
        return 0;
    }

    if (entities_by_id.size() < SV_MAX_ENTITIES) {
        for (int i = 1; i < entities.size(); i++) {
            if (!entities[i].id) {
                entity = &entities[i];
                entity->id = entity_id;
                entity->type = type;
                entity->spawned_at = now;
                entity->ghost = &ghosts[i];
                entities_by_id[entity_id] = i;
                break;
            }
        }
    }

    if (!entity) {
        printf("[entity_db] Failed to spawn entity id %u of type %s. Max entities.\n", entity_id, EntityTypeStr(type));
    }
    return entity;
}
bool EntityDB::DespawnEntity(uint32_t entity_id, double now)
{
    assert(entity_id);

    Entity *entity = FindEntity(entity_id);
    assert(!entity->despawned_at);  // in case we change how FindEntity works
    if (entity) {
        entity->despawned_at = now;
        return true;
    } else {
        printf("[entity_db] Failed to despawn entity id %u. Entity id not found.\n", entity_id);
    }
    return false;
}
void EntityDB::DestroyEntity(uint32_t entity_id)
{
    assert(entity_id);

    Entity *entity = FindEntity(entity_id, true);
    if (entity) {
        assert(entity->id);   // wtf happened man
        assert(entity->type); // wtf happened man

        // Clear aspects
        *entity->ghost = {};
        *entity = {};

        entities_by_id.erase(entity_id);
    } else {
        assert(0);
        printf("error: entity_id %u out of range\n", entity_id);
    }
}

void EntityDB::EntityTick(Entity &entity, double dt, double now)
{
    Vector3 pos = entity.position;
    Vector3 vel = entity.velocity;

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
    if (Vector3LengthSqr(vel) < 0.1f * 0.1f) {
        vel = Vector3Zero();
    }
#endif

    pos = Vector3Add(pos, Vector3Scale(vel, dt));

    if (!Vector3Equals(pos, entity.position) || Vector3LengthSqr(entity.velocity) != 0) {
        entity.velocity = vel;
        entity.position = pos;
        entity.last_moved_at = now;
    }
}

void EntityDB::DrawEntityId(Entity &entity, Camera2D &camera)
{
    const char *text = TextFormat("%u", entity.id);
    Vector2 pos = GetWorldToScreen2D(entity.Position2D(), camera);
    Vector2 textSize = dlb_MeasureTextEx(fntMedium, CSTRLEN(text));
    Vector2 textPos{
        floorf(pos.x - textSize.x / 2.0f),
        pos.y
    };
    dlb_DrawTextShadowEx(fntMedium, CSTRLEN(text), pos, WHITE);
}
void EntityDB::DrawEntity(Entity &entity, DrawCmdQueue &sortedDraws, bool highlight)
{
    const Rectangle rect = entity.GetSpriteRect();
    DrawSprite(entity, &sortedDraws, highlight);
}