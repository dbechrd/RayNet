#include "entity.h"

void Entity::Serialize(EntitySnapshot &entitySnapshot, double serverTime, uint32_t entityId)
{
    entitySnapshot.serverTime = serverTime;
    entitySnapshot.id = entityId;
    entitySnapshot.type = type;
    entitySnapshot.color = color;
    entitySnapshot.size = size;
    entitySnapshot.speed = speed;
    entitySnapshot.velocity = velocity;
    entitySnapshot.position = position;
}

void Entity::ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha)
{
    // TODO: Lerp at least position, also maybe the others
    type = b.type;
    color = b.color;
    size = b.size;
    speed = b.speed;
    velocity = b.velocity;
    position.x = LERP(a.position.x, b.position.x, (float)alpha);
    position.y = LERP(a.position.y, b.position.y, (float)alpha);
}

void Entity::Draw(const Font &font, int clientIdx) {
    DrawRectanglePro(
        { position.x, position.y, size.x, size.y },
        { size.x / 2, size.y },
        0,
        color
    );
    const char *str = TextFormat("%d", clientIdx);
    Vector2 strSize = MeasureTextEx(font, str, (float)FONT_SIZE, 1.0f);
    DrawTextShadowEx(font, str,
        {
            position.x - strSize.x / 2,
            position.y - strSize.y
        },
        (float)FONT_SIZE, RAYWHITE
    );
}