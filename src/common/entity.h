#pragma once
#include "input_command.h"
#include "net/entity_snapshot.h"
#include "ring_buffer.h"

enum EntityType {
    Entity_None,
    Entity_Player,
    Entity_Bot,
    Entity_Count,
};

struct Entity {
    EntityType type;
    Color color;
    Vector2 size;

    float speed;
    Vector2 velocity;
    Vector2 position;

    void Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd);
    void Tick(const InputCmd *input, double dt);
    void ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha);
    void Draw(const Font &font, int clientIdx);
};