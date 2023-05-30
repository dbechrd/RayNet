#pragma once
#include "common.h"
#include "../common/ring_buffer.h"

struct Msg_S_EntitySnapshot;

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

struct EntityCollisionTuple {
    Entity          &entity;
    AspectCollision &collision;
    AspectPhysics   &physics;

    EntityCollisionTuple(Entity &entity, AspectCollision &collision, AspectPhysics &physics)
        : entity(entity), collision(collision), physics(physics) {}
};

struct EntityInterpolateTuple {
    Entity        &entity;
    AspectPhysics &physics;
    AspectLife    &life;

    EntityInterpolateTuple(Entity &entity, AspectPhysics &physics, AspectLife &life)
        : entity(entity), physics(physics), life(life) {}
};

struct EntitySpriteTuple {
    Entity       &entity;
    data::Sprite &sprite;

    EntitySpriteTuple(Entity &entity, data::Sprite &sprite)
        : entity(entity), sprite(sprite) {}
};

struct EntityTickTuple {
    Entity        &entity;
    AspectDialog  &dialog;
    AspectPhysics &physics;

    EntityTickTuple(Entity &entity, AspectDialog &dialog, AspectPhysics &physics)
        : entity(entity), dialog(dialog), physics(physics) {}
};

struct EntityData {
    Entity          entity   {};
    AspectCollision collision{};
    AspectDialog    dialog   {};
    AspectGhost     ghosts   {};
    AspectLife      life     {};
    AspectPathfind  pathfind {};
    AspectPhysics   physics  {};
    data::Sprite    sprite   {};
};
//
//struct EntityDataRef {
//    Entity          &entity;
//    AspectCollision &collision;
//    AspectDialog    &dialog;
//    AspectGhost     &ghosts;
//    AspectLife      &life;
//    AspectPathfind  &pathfind;
//    AspectPhysics   &physics;
//    data::Sprite    &sprite;
//
//    EntityDataRef(
//        Entity          &entity,
//        AspectCollision &collision,
//        AspectDialog    &dialog,
//        AspectGhost     &ghosts,
//        AspectLife      &life,
//        AspectPathfind  &pathfind,
//        AspectPhysics   &physics,
//        data::Sprite    &sprite
//    ) :
//        entity    (entity),
//        collision (collision),
//        dialog    (dialog),
//        ghosts    (ghosts),
//        life      (life),
//        pathfind  (pathfind),
//        physics   (physics),
//        sprite    (sprite)
//    {}
//};