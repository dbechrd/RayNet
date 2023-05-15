#pragma once
#include "common.h"
#include "spritesheet.h"

struct EntitySnapshot;
struct EntitySpawnEvent;

enum EntityType {
    Entity_None,
    Entity_Player,
    Entity_Bot,
    Entity_Projectile,
    Entity_Count,
};

struct EntityLife {
    int maxHealth{};
    int health{};

    void TakeDamage(int damage) {
        if (damage >= health) {
            health = 0;
        } else {
            health -= damage;
        }
    }

    bool Alive(void) {
        return health > 0;
    }

    bool Dead(void) {
        return !Alive();
    }
};

struct EntityPlayer {
    EntityLife life{};
    uint32_t playerId{};
};

struct EntityBot {
    EntityLife life{};
    int pathId{};
    int pathNodeLastArrivedAt{};
    int pathNodeTarget{};
    double pathNodeArrivedAt{};
};

struct Entity {
    EntityType type;
    double spawnedAt;
    double despawnedAt;

    Color color;
    Vector2 size;
    float radius;    // collision
    bool colliding;  // not sync'd, local flag for debugging colliders

    float drag;
    float speed;
    Vector2 forceAccum;
    Vector2 velocity;
    Vector2 position;

    SpriteId spriteId;

    // TODO(dlb): Could be a pointer, or could be a type + index into another pool
    union {
        EntityPlayer player;
        EntityBot bot;
    } data;

    uint32_t latestDialog;

    uint32_t freelist_next;

    void Serialize(uint32_t entityId, EntitySpawnEvent &entitySpawnEvent, double serverTime);
    void Serialize(uint32_t entityId, EntitySnapshot &entitySnapshot, double serverTime, uint32_t lastProcessedInputCmd);
    void ApplySpawnEvent(const EntitySpawnEvent &spawnEvent);
    void ApplyStateInterpolated(const EntitySnapshot &a, const EntitySnapshot &b, double alpha);
    void ApplyForce(Vector2 force);
    void Tick(double dt);
    Rectangle GetRect(void);
    EntityLife *GetLife(void);
    void DrawHoverInfo(void);
    void Draw(const Font &font, int clientIdx, float scale);
};