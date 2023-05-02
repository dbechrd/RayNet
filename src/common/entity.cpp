#include "entity.h"

#define SPAWN_PROP(prop) entitySpawnEvent.entity.prop = prop;
void Entity::Serialize(uint32_t entityId, EntitySpawnEvent &entitySpawnEvent, double serverTime)
{
    entitySpawnEvent.serverTime = serverTime;
    entitySpawnEvent.id = entityId;

    SPAWN_PROP(type);
    SPAWN_PROP(color);
    SPAWN_PROP(size);
    SPAWN_PROP(radius);
    SPAWN_PROP(drag);
    SPAWN_PROP(speed);
    SPAWN_PROP(velocity);
    SPAWN_PROP(position);
    switch (type) {
        case Entity_Player: {
            SPAWN_PROP(data.player.life.maxHealth);
            SPAWN_PROP(data.player.life.health);
            break;
        }
        case Entity_Bot: {
            SPAWN_PROP(data.bot.life.maxHealth);
            SPAWN_PROP(data.bot.life.health);
            break;
        }
    }
}
#undef SPAWN_PROP

#define SNAPSHOT_PROP(prop) entitySnapshot.entity.prop = prop;
void Entity::Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd)
{
    entitySnapshot.serverTime = serverTime;
    entitySnapshot.lastProcessedInputCmd = lastProcessedInputCmd;
    entitySnapshot.id = entityId;

    SNAPSHOT_PROP(type);
    SNAPSHOT_PROP(velocity);
    SNAPSHOT_PROP(position);
    switch (type) {
        case Entity_Player: {
            SNAPSHOT_PROP(data.player.life.maxHealth);
            SNAPSHOT_PROP(data.player.life.health);
            break;
        }
        case Entity_Bot: {
            SNAPSHOT_PROP(data.bot.life.maxHealth);
            SNAPSHOT_PROP(data.bot.life.health);
            break;
        }
    }
}
#undef SNAPSHOT_PROP

#define APPLY_PROP(prop) prop = entitySpawnEvent.entity.prop;
void Entity::ApplySpawnEvent(const EntitySpawnEvent &entitySpawnEvent)
{
    APPLY_PROP(type);
    APPLY_PROP(color);
    APPLY_PROP(size);
    APPLY_PROP(radius);
    APPLY_PROP(drag);
    APPLY_PROP(speed);
    APPLY_PROP(velocity);
    APPLY_PROP(position);
    switch (type) {
        case Entity_Player: {
            APPLY_PROP(data.player.life.maxHealth);
            APPLY_PROP(data.player.life.health);
            break;
        }
        case Entity_Bot: {
            APPLY_PROP(data.bot.life.maxHealth);
            APPLY_PROP(data.bot.life.health);
            break;
        }
    }
}
#undef APPLY_PROP

#define LERP_PROP(prop) prop = LERP(a.entity.prop, b.entity.prop, (float)alpha);
void Entity::ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha)
{
#if 0
    color.r = LERP(a.entity.color.r, b.entity.color.r, (float)alpha);
    color.g = LERP(a.entity.color.g, b.entity.color.g, (float)alpha);
    color.b = LERP(a.entity.color.b, b.entity.color.b, (float)alpha);
    color.a = LERP(a.entity.color.a, b.entity.color.a, (float)alpha);

    size.x = LERP(a.entity.size.x, b.entity.size.x, (float)alpha);
    size.y = LERP(a.entity.size.y, b.entity.size.y, (float)alpha);

    radius = LERP(a.entity.radius, b.entity.radius, (float)alpha);

    drag = LERP(a.entity.drag, b.entity.drag, (float)alpha);
    speed = LERP(a.entity.speed, b.entity.speed, (float)alpha);
#endif
    LERP_PROP(velocity.x);
    LERP_PROP(velocity.y);
    LERP_PROP(position.x);
    LERP_PROP(position.y);
    switch (type) {
        case Entity_Player: {
            LERP_PROP(data.player.life.maxHealth);
            LERP_PROP(data.player.life.health);
            break;
        }
        case Entity_Bot: {
            LERP_PROP(data.bot.life.maxHealth);
            LERP_PROP(data.bot.life.health);

            if (data.bot.life.health > 100) {
                __debugbreak();
            }

            break;
        }
    }
}
#undef LERP_PROP

void Entity::ApplyForce(Vector2 force)
{
    forceAccum.x += force.x;
    forceAccum.y += force.y;
}

void Entity::Tick(double now, double dt)
{
    velocity.x += forceAccum.x;
    velocity.y += forceAccum.y;
    forceAccum = {};

    velocity.x *= (1.0f - drag * dt);
    velocity.y *= (1.0f - drag * dt);

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
}

Rectangle Entity::GetRect(void)
{
    Rectangle rect{ position.x - size.x / 2, position.y - size.y, size.x, size.y };
    return rect;
}

EntityLife *Entity::GetLife(void)
{
    EntityLife *life = 0;
    switch (type) {
        case Entity_Player: {
            life = &data.player.life;
            break;
        }
        case Entity_Bot: {
            life = &data.bot.life;
            break;
        }
    }
    return life;
}

void Entity::DrawHoverInfo(void)
{
    EntityLife *life = GetLife();

    if (life && life->maxHealth) {
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
            (float)GetScreenWidth() / 2 - hpBarSize.x / 2 - hpBarPad.x,
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
        float pctHealth = CLAMP((float)life->health / life->maxHealth, 0, 1);
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
void Entity::Draw(const Font &font, int clientIdx, float scale) {
    if (!color.a) {
        return;
    }

    // Scale
    float sx = size.x * scale;
    float sy = size.y * scale;

    // Position (after scaling)
    float x = position.x;
    float y = position.y - (size.y - sy) / 2;

    if (type == Entity_Bot) {
        DrawTextureV(texLily, { position.x - texLily.width / 2, position.y - texLily.height }, WHITE);
    } else {
#if 0
#if 1
    DrawRectanglePro(
        { x, y, sx, sy },
        { sx / 2, sy },
        0,
        color
    );
#else
    // Windows Logo
    DrawRectanglePro(
        { x, y, sx, sy },
        { sx / 2, sy },
        0,
        RED
    );
    DrawRectangleLinesEx({ x - sx / 2, y - sy, sx, sy }, 10, BLACK);

    DrawRectanglePro(
        { x + sx, y, sx, sy },
        { sx / 2, sy },
        0,
        GREEN
    );
    DrawRectangleLinesEx({ x + sx - sx / 2, y - sy, sx, sy }, 10, BLACK);

    DrawRectanglePro(
        { x, y + sy, sx, sy },
        { sx / 2, sy },
        0,
        BLUE
    );
    DrawRectangleLinesEx({ x - sx / 2, y + sy - sy, sx, sy }, 10, BLACK);

    DrawRectanglePro(
        { x + sx, y + sy, sx, sy },
        { sx / 2, sy },
        0,
        YELLOW
    );
    DrawRectangleLinesEx({ x + sx - sx / 2, y + sy - sy, sx, sy }, 10, BLACK);
#endif

#else
    DrawRectangleRounded(
        { x - sx / 2, y - sy, sx, sy },
        0.5f,
        4,
        color
    );
#endif
    }

#if CL_DBG_SNAPSHOT_IDS
    const char *str = TextFormat("Bob #%d", clientIdx);
    Vector2 strSize = MeasureTextEx(font, str, (float)font.baseSize, 1.0f);
    DrawTextShadowEx(font, str,
        {
            position.x - strSize.x / 2,
            position.y - sy - strSize.y
        },
        (float)font.baseSize, RAYWHITE
    );
#endif
}