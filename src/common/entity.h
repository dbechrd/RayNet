#pragma once
#include "input_command.h"
#include "net/entity_snapshot.h"
#include "ring_buffer.h"

enum EntityType {
    Entity_None,
    Entity_Player,
    Entity_Bot,
    Entity_Projectile,
    Entity_Count,
};

struct Entity {
    EntityType type;
    bool despawnedAt;

    uint32_t index;
    Color color;
    Vector2 size;
    float radius;  // collision

    float drag;
    float speed;
    Vector2 forceAccum;
    Vector2 velocity;
    Vector2 position;

    uint32_t freelist_next;

    void Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd);
    void ApplyForce(Vector2 force);
    void Tick(double dt);
    void ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha);
    void Draw(const Font &font, int clientIdx, float scale);
};