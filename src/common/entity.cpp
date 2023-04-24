#include "entity.h"

void Entity::Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd)
{
    entitySnapshot.serverTime = serverTime;
    entitySnapshot.lastProcessedInputCmd = lastProcessedInputCmd;
    entitySnapshot.id = entityId;
    entitySnapshot.type = type;
    entitySnapshot.color = color;
    entitySnapshot.size = size;
    entitySnapshot.radius = radius;
    entitySnapshot.drag = drag;
    entitySnapshot.speed = speed;
    entitySnapshot.velocity = velocity;
    entitySnapshot.position = position;
}

void Entity::ApplyForce(Vector2 force)
{
    forceAccum.x += force.x;
    forceAccum.y += force.y;
}

void Entity::Tick(double dt)
{
    velocity.x += forceAccum.x;
    velocity.y += forceAccum.y;
    forceAccum = {};

    velocity.x *= (1.0f - drag * dt);
    velocity.y *= (1.0f - drag * dt);

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
}

void Entity::ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha)
{
    type = b.type;

    color.r = LERP(a.color.r, b.color.r, (float)alpha);
    color.g = LERP(a.color.g, b.color.g, (float)alpha);
    color.b = LERP(a.color.b, b.color.b, (float)alpha);
    color.a = LERP(a.color.a, b.color.a, (float)alpha);

    size.x = LERP(a.size.x, b.size.x, (float)alpha);
    size.y = LERP(a.size.y, b.size.y, (float)alpha);

    radius = LERP(a.radius, b.radius, (float)alpha);

    drag = LERP(a.drag, b.drag, (float)alpha);
    speed = LERP(a.speed, b.speed, (float)alpha);

    velocity.x = LERP(a.velocity.x, b.velocity.x, (float)alpha);
    velocity.y = LERP(a.velocity.y, b.velocity.y, (float)alpha);

    position.x = LERP(a.position.x, b.position.x, (float)alpha);
    position.y = LERP(a.position.y, b.position.y, (float)alpha);
}

// TODO: Refactor out into drawrectangle or some shit
void Entity::Draw(const Font &font, int clientIdx, float scale) {
    if (!color.a) {
        return;
    }

#if 1
    // Scale
    float sx = size.x * scale;
    float sy = size.y * scale;

    // Position (after scaling)
    float x = position.x;
    float y = position.y - (size.y - sy) / 2;

#if 1
    DrawRectanglePro(
        { x, y, sx, sy },
        { sx / 2, sy },
        0,
        color
    );
#else
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
        { position.x, position.y, size.x, size.y },
        0.5f,
        4,
        color
    );
#endif

#if CL_DBG_SNAPSHOT_IDS
    const char *str = TextFormat("%d", clientIdx);
    Vector2 strSize = MeasureTextEx(font, str, (float)font.baseSize, 1.0f);
    DrawTextShadowEx(font, str,
        {
            position.x - strSize.x / 2,
            position.y - strSize.y
        },
        (float)font.baseSize, RAYWHITE
    );
#endif
}