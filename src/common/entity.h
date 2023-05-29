#pragma once
#include "common.h"
#include "../common/ring_buffer.h"

struct Msg_S_EntitySnapshot;

struct AspectCollision {
    float radius    {};  // collision
    bool  colliding {};  // not sync'd, local flag for debugging colliders
};

struct AspectDialog {
    double   spawnedAt     {};  // time when dialog was spawned
    uint32_t messageLength {};  // how much they're saying
    char *   message       {};  // what they're saying

    ~AspectDialog(void) {
        if (message) {
            free(message);
        }
    }
};

struct GhostSnapshot {
    double   serverTime {};

    // Entity
    Vector2  position   {};

    // Physics
    float    speed      {};
    Vector2  velocity   {};

    // Life
    int      maxHealth  {};
    int      health     {};

    // TODO: Wtf do I do with this shit?
    uint32_t lastProcessedInputCmd {};

    GhostSnapshot(void) {}
    GhostSnapshot(Msg_S_EntitySnapshot &msg);
};

typedef RingBuffer<GhostSnapshot, CL_SNAPSHOT_COUNT> AspectGhost;

struct AspectLife {
    int maxHealth;
    int health;

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

struct AspectPathfind {
    int pathId;
    int pathNodeLastArrivedAt;
    int pathNodeTarget;
    double pathNodeArrivedAt;
};

struct AspectPhysics {
    float drag;
    float speed;
    Vector2 forceAccum;
    Vector2 velocity;

    void ApplyForce(Vector2 force) {
        forceAccum.x += force.x;
        forceAccum.y += force.y;
    }
};

enum EntityType {
    Entity_None,
    Entity_Player,
    Entity_NPC,
    Entity_Projectile,
    Entity_Count,
};

struct Entity {
    uint32_t id;
    EntityType type;
    double spawnedAt;
    double despawnedAt;
    Vector2 position;

    // TODO: Separate this out into its own array?
    uint32_t freelist_next;
};

#if 0
// A random idea that probably makes no sense:
struct Player : public Entity {
    AspectCollision collision;
    AspectLife      life;
    AspectPhysics   physics;
};

struct NPC : public Entity {
    AspectCollision collision;
    AspectDialog    dialog;
    AspectLife      life;
    AspectPathfind  pathfind;
    AspectPhysics   physics;
};

struct Projectile : public Entity {
    AspectCollision collision;
    AspectPhysics   physics;
};
#endif