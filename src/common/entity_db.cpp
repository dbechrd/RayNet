#include "entity_db.h"

EntityDB *entityDb{};

EntityDB::EntityDB(void)
{
    entity_freelist = 1;  // 0 is reserved for null entity
    for (uint32_t i = 1; i < entities.size() - 1; i++) {
        Entity &entity = entities[i];
        entities[i].freelist_next = i + 1;
    }
}

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
Entity *EntityDB::FindEntity(uint32_t entityId, bool deadOrAlive)
{
    size_t entityIndex = FindEntityIndex(entityId);
    if (entityIndex) {
        Entity &entity = entities[entityIndex];
        assert(entity.id == entityId);
        assert(entity.type);
        if (deadOrAlive || !entity.despawnedAt) {
            return &entity;
        }
    }
    return 0;
}
bool EntityDB::SpawnEntity(uint32_t entityId, EntityType entityType, double now)
{
    assert(entityId);
    assert(entityType);

    Entity *entity = FindEntity(entityId);
    if (entity) {
        assert(0);
        printf("[entity_db] Failed to spawn entity id %u of type %s. An entity with that id already exists.\n", entityId, EntityTypeStr(entityType));
        return false;
    }

    if (entity_freelist) {
        size_t entityIndex = entity_freelist;
        Entity &e = entities[entityIndex];
        entity_freelist = e.freelist_next;

        e.freelist_next = 0;
        e.id = entityId;
        e.type = entityType;
        e.spawnedAt = now;
        entityIndexById[entityId] = entityIndex;
        return true;
    } else {
        printf("[entity_db] Failed to spawn entity id %u of type %s. Freelist is empty.\n", entityId, EntityTypeStr(entityType));
        return false;
    }
}
bool EntityDB::DespawnEntity(uint32_t entityId, double now)
{
    assert(entityId);

    Entity *entity = FindEntity(entityId);
    if (entity && !entity->despawnedAt) {
        entity->despawnedAt = now;
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
        Entity &entity = entities[entityIndex];
        assert(entity.id);   // wtf happened man
        assert(entity.type); // wtf happened man

        // Clear aspects
        entities  [entityIndex] = {};
        collision [entityIndex] = {};
        dialog    [entityIndex] = {};
        ghosts    [entityIndex] = {};
        life      [entityIndex] = {};
        pathfind  [entityIndex] = {};
        physics   [entityIndex] = {};
        sprite    [entityIndex] = {};

        // Remove from map
        entityIndexById.erase(entityId);

        // Update freelist
        entity.freelist_next = entity_freelist;
        entity_freelist = entityIndex;
    } else {
        assert(0);
        printf("error: entityId %u out of range\n", entityId);
    }
}

Rectangle EntityDB::EntityRect(EntitySpriteTuple &data)
{
    const data::GfxFrame &frame = data::GetSpriteFrame(data.sprite);
    const Rectangle rect{
        data.entity.position.x - (float)(frame.w / 2),
        data.entity.position.y - (float)frame.h,
        (float)frame.w,
        (float)frame.h
    };
    return rect;
}

Rectangle EntityDB::EntityRect(uint32_t entityId)
{
    assert(entityId);

    size_t entityIndex = FindEntityIndex(entityId);
    if (entityIndex) {
        Entity &entity = entities[entityIndex];
        data::Sprite &sprite = this->sprite[entityIndex];
        EntitySpriteTuple data{ entity, sprite };
        return EntityRect(data);
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
void EntityDB::EntityTick(EntityTickTuple &data, double dt)
{
    data.physics.velocity.x += data.physics.forceAccum.x * dt;
    data.physics.velocity.y += data.physics.forceAccum.y * dt;
    data.physics.forceAccum = {};

#if 1
    const float drag = (1.0f - data.physics.drag * dt);
    data.physics.velocity.x *= drag;
    data.physics.velocity.y *= drag;
#elif 0
    const float drag = (1.0f - data.physics.drag * SV_TICK_DT);
    if (dt == SV_TICK_DT) {
        data.physics.velocity.x *= drag;
        data.physics.velocity.y *= drag;
    } else {
        //const float alpha = dt / SV_TICK_DT;
        //data.physics.velocity.x *= drag * alpha;
        //data.physics.velocity.y *= drag * alpha;
    }
#else
    data.physics.velocity.x *= 1.0f - exp2f(-10.0f * data.physics.drag * dt);
    data.physics.velocity.y *= 1.0f - exp2f(-10.0f * data.physics.drag * dt);
#endif

    data.entity.position.x += data.physics.velocity.x * dt;
    data.entity.position.y += data.physics.velocity.y * dt;
}
void EntityDB::EntityTick(uint32_t entityId, double dt)
{
    assert(entityId);

    size_t entityIndex = FindEntityIndex(entityId);
    if (!entityIndex) return;

    Entity &entity = entities[entityIndex];
    AspectLife &life = this->life[entityIndex];
    AspectPhysics &physics = this->physics[entityIndex];
    data::Sprite &sprite = this->sprite[entityIndex];

    EntityTickTuple data{ entity, life, physics, sprite };
    EntityTick(data, dt);
}

void EntityDB::DrawEntityIds(uint32_t entityId, Camera2D &camera)
{
    assert(entityId);
    size_t entityIndex = FindEntityIndex(entityId);
    if (!entityIndex) return;

    Entity &entity = entities[entityIndex];
    if (!entity.id || !entity.type) {
        assert(!entity.id && !entity.type);
        return;
    }

    DrawTextEx(fntHackBold20, TextFormat("%u", entity.id), entity.position,
        fntHackBold20.baseSize, 1, WHITE);
}
void EntityDB::DrawEntityHoverInfo(uint32_t entityId)
{
    assert(entityId);

    size_t entityIndex = FindEntityIndex(entityId);
    if (!entityIndex) return;

    AspectLife &life = this->life[entityIndex];
    if (!life.maxHealth) return;

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

    if (fabsf(life.health - life.healthSmooth) < 1.0f) {
        float pctHealth = CLAMP((float)life.healthSmooth / life.maxHealth, 0, 1);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctHealth), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    } else {
        float pctHealth = CLAMP((float)life.health / life.maxHealth, 0, 1);
        float pctHealthSmooth = CLAMP((float)life.healthSmooth / life.maxHealth, 0, 1);
        float pctWhite = MAX(pctHealth, pctHealthSmooth);
        float pctRed = MIN(pctHealth, pctHealthSmooth);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctWhite), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(WHITE, -0.3));
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctRed), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));
    }

    Vector2 labelSize = MeasureTextEx(fntHackBold20, "Lily", fntHackBold20.baseSize, 1);
    Vector2 labelPos{
        floorf(hpBarBg.x + hpBarBg.width / 2 - labelSize.x / 2),
        floorf(hpBarBg.y + hpBarBg.height / 2 - labelSize.y / 2)
    };
    DrawTextShadowEx(fntHackBold20, "Lily", labelPos, WHITE);
}
void EntityDB::DrawEntity(uint32_t entityId)
{
    size_t entityIndex = FindEntityIndex(entityId);
    if (entityIndex) {
        Entity &entity = entities[entityIndex];
        data::Sprite &sprite = this->sprite[entityIndex];
        EntitySpriteTuple data{ entity, sprite };
        const Rectangle rect = EntityRect(data);
        data::DrawSprite(sprite, { rect.x, rect.y });
    }
}