#include "entity.h"
#include "net/net.h"
#include "tilemap.h"

void Entity::ApplyForce(Tilemap &map, uint32_t entityId, Vector2 force)
{
    AspectPhysics &physics = map.physics[entityId];

    physics.forceAccum.x += force.x;
    physics.forceAccum.y += force.y;
}

void Entity::Tick(Tilemap &map, uint32_t entityId, double dt)
{
    Entity &entity = map.entities[entityId];
    AspectPhysics &physics = map.physics[entityId];

    physics.velocity.x += physics.forceAccum.x * dt;
    physics.velocity.y += physics.forceAccum.y * dt;
    physics.forceAccum = {};

#if 1
    physics.velocity.x *= (1.0f - physics.drag * dt);
    physics.velocity.y *= (1.0f - physics.drag * dt);
#else
    physics.velocity.x *= 1.0f - powf(physics.drag, dt);
    physics.velocity.y *= 1.0f - powf(physics.drag, dt);
#endif

    entity.position.x += physics.velocity.x * dt;
    entity.position.y += physics.velocity.y * dt;
}

Rectangle Entity::GetRect(Tilemap &map, uint32_t entityId)
{
    Entity &entity = map.entities[entityId];
    AspectSprite &sprite = map.sprite[entityId];

    const Vector2 size = sprite.GetSize();
    const Rectangle rect{ entity.position.x - size.x / 2, entity.position.y - size.y, size.x, size.y };
    return rect;
}

Vector2 Entity::TopCenter(Tilemap &map, uint32_t entityId)
{
    const Rectangle rect = GetRect(map, entityId);
    const Vector2 topCenter{
        rect.x + rect.width / 2,
        rect.y
    };
    return topCenter;
}

void Entity::DrawHoverInfo(Tilemap &map, uint32_t entityId)
{
    AspectLife &life = map.life[entityId];
    if (life.maxHealth) {
#if 0
        Rectangle hpBar{
            position.x - maxHealth / 2,
            position.y - size.y - 10,
            maxHealth,
            10
        };
#else
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

#endif
        DrawRectangleRec(hpBarBg, Fade(BLACK, 0.5));
        float pctHealth = CLAMP((float)life.health / life.maxHealth, 0, 1);
        hpBar.width = CLAMP(ceilf(hpBarSize.x * pctHealth), 0, hpBarSize.x);
        DrawRectangleRec(hpBar, ColorBrightness(MAROON, -0.4));

        Vector2 labelSize = MeasureTextEx(fntHackBold20, "Lily", fntHackBold20.baseSize, 1);
        Vector2 labelPos{
            floorf(hpBarBg.x + hpBarBg.width / 2 - labelSize.x / 2),
            floorf(hpBarBg.y + hpBarBg.height / 2 - labelSize.y / 2)
        };
        DrawTextShadowEx(fntHackBold20, "Lily", labelPos, WHITE);
    }
}

// TODO: Refactor out into drawrectangle or some shit
void Entity::Draw(Tilemap &map, uint32_t entityId, double now) {
    AspectSprite &sprite = map.sprite[entityId];

    const Rectangle rect = GetRect(map, entityId);
    sprite.Draw({ rect.x, rect.y }, 1.0f, now);
}