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

struct EntityBot {
    uint32_t pathId = 0;  // TODO: Entity should remember which path it follows somehow
    int pathNodeIndexPrev = 0;
    int pathNodeIndexTarget = 0;
    double pathNodeArrivedAt = 0;
};

struct Entity {
    EntityType type;
    double spawnedAt;
    double despawnedAt;

    uint32_t index;  // pool index?
    Color color;
    Vector2 size;
    float radius;    // collision
    bool colliding;  // not sync'd, local flag for debugging colliders

    float drag;
    float speed;
    Vector2 forceAccum;
    Vector2 velocity;
    Vector2 position;

    // TODO(dlb): Could be a pointer, or could be a type + index into another pool
    union {
        EntityBot bot{};
    } data;

    uint32_t freelist_next;

    void Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd);
    void ApplyForce(Vector2 force);
    void Tick(double dt);
    void ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha);
    void Draw(const Font &font, int clientIdx, float scale);
};