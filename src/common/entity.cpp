#include "entity.h"

void Entity::Serialize(EntityState &entityState)
{
    entityState.color = color;
    entityState.size = size;
    entityState.speed = speed;
    entityState.velocity = velocity;
    entityState.position = position;
}

void Entity::ApplyStateInterpolated(EntityState &a, EntityState &b, double alpha)
{
    // TODO: Lerp at least position, also maybe the others
    color = b.color;
    size = b.size;
    speed = b.speed;
    velocity = b.velocity;
    position.x = LERP(a.position.x, b.position.x, alpha);
    position.y = LERP(a.position.y, b.position.y, alpha);
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