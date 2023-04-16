#include "entity.h"

void Entity::Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd)
{
    entitySnapshot.serverTime = serverTime;
    entitySnapshot.lastProcessedInputCmd = lastProcessedInputCmd;
    entitySnapshot.id = entityId;
    entitySnapshot.type = type;
    entitySnapshot.color = color;
    entitySnapshot.size = size;
    entitySnapshot.speed = speed;
    entitySnapshot.velocity = velocity;
    entitySnapshot.position = position;
}

void Entity::Tick(const InputCmd *input, double dt) {
    Vector2 accelDelta{};

    if (input) {
        if (input->north) {
            accelDelta.y -= 1.0f;
        }
        if (input->west) {
            accelDelta.x -= 1.0f;
        }
        if (input->south) {
            accelDelta.y += 1.0f;
        }
        if (input->east) {
            accelDelta.x += 1.0f;
        }
        if (accelDelta.x && accelDelta.y) {
            float invLength = 1.0f / sqrtf(accelDelta.x * accelDelta.x + accelDelta.y * accelDelta.y);
            accelDelta.x *= invLength;
            accelDelta.y *= invLength;
        }
    } else {
        printf("No input\n");
    }

    velocity.x += accelDelta.x * speed;
    velocity.y += accelDelta.y * speed;

    velocity.x *= 0.5f;
    velocity.y *= 0.5f;

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

    speed = LERP(a.speed, b.speed, (float)alpha);

    velocity.x = LERP(a.velocity.x, b.velocity.x, (float)alpha);
    velocity.y = LERP(a.velocity.y, b.velocity.y, (float)alpha);

    position.x = LERP(a.position.x, b.position.x, (float)alpha);
    position.y = LERP(a.position.y, b.position.y, (float)alpha);
}

void Entity::Draw(const Font &font, int clientIdx) {
#if 1
    DrawRectanglePro(
        { position.x, position.y, size.x, size.y },
        { size.x / 2, size.y },
        0,
        color
    );
#else
    DrawRectangleRounded(
        { position.x, position.y, size.x, size.y },
        0.5f,
        4,
        color
    );
#endif
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