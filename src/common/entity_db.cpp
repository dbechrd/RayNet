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
void EntityDB::EntityTick(uint32_t entityId, double dt)
{
    assert(entityId);

    data::Entity *entity = FindEntity(entityId);
    if (entity) {
        EntityTick(*entity, dt);
    }
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
void EntityDB::DrawEntityHoverInfo(uint32_t entityId)
{
    assert(entityId);

    data::Entity *entity = FindEntity(entityId);
    if (!entity) return;
    if (!entity->hp_max) return;

    const float borderWidth = 1;
    const float pad = 1;
    Vector2 hpBarPad{ pad, pad };
    Vector2 hpBarSize{ 200, 24 };

    Rectangle hpBarBg{
        (float)GetRenderWidth() / 2 - hpBarSize.x / 2 - hpBarPad.x,
        20.0f,
        hpBarSize.x + hpBarPad.x * 2,
        hpBarSize.y + hpBarPad.y * 2
    };

    Rectangle hpBar{
        hpBarBg.x + hpBarPad.x,
        hpBarBg.y + hpBarPad.y,
        hpBarSize.x,
        hpBarSize.y
    };

    DrawRectangleRec(hpBarBg, Fade(BLACK, 0.5));

    if (fabsf(entity->hp - entity->hp_smooth) < 1.0f) {
        float pctHealth = CLAMP((float)entity->hp_smooth / entity->hp_max, 0, 1);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctHealth), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    } else {
        float pctHealth = CLAMP((float)entity->hp / entity->hp_max, 0, 1);
        float pctHealthSmooth = CLAMP((float)entity->hp_smooth / entity->hp_max, 0, 1);
        float pctWhite = MAX(pctHealth, pctHealthSmooth);
        float pctRed = MIN(pctHealth, pctHealthSmooth);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctWhite), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(WHITE, -0.3));
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctRed), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    }

    Vector2 labelSize = MeasureTextEx(fntMedium, entity->name.c_str(), fntMedium.baseSize, 1);
    Vector2 labelPos{
        floorf(hpBarBg.x + hpBarBg.width / 2 - labelSize.x / 2),
        floorf(hpBarBg.y + hpBarBg.height / 2 - labelSize.y / 2)
    };
    DrawTextShadowEx(fntMedium, entity->name.c_str(), labelPos, WHITE);
}
void EntityDB::DrawEntity(uint32_t entityId, data::DrawCmdQueue &sortedDraws)
{
    data::Entity *entity = FindEntity(entityId);
    if (entity) {
        const Rectangle rect = data::GetSpriteRect(*entity);
        data::DrawSprite(*entity, &sortedDraws);
    }
}