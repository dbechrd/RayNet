#pragma once
#include "common.h"
#include "../common/ring_buffer.h"

struct Msg_S_EntitySnapshot;

struct GhostSnapshot {
    double   serverTime {};

    // Entity
    uint32_t mapId      {};
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
    data::Entity          &entity;
    data::AspectCollision &collision;
    data::AspectPhysics   &physics;

    EntityCollisionTuple(data::Entity &entity, data::AspectCollision &collision, data::AspectPhysics &physics)
        : entity(entity), collision(collision), physics(physics) {}
};

struct EntityInterpolateTuple {
    data::Entity        &entity;
    data::AspectPhysics &physics;
    data::AspectLife    &life;
    data::Sprite        &sprite;

    EntityInterpolateTuple(data::Entity &entity, data::AspectPhysics &physics, data::AspectLife &life, data::Sprite &sprite)
        : entity(entity), physics(physics), life(life), sprite(sprite) {}
};

struct EntitySpriteTuple {
    data::Entity &entity;
    data::Sprite &sprite;

    EntitySpriteTuple(data::Entity &entity, data::Sprite &sprite)
        : entity(entity), sprite(sprite) {}
};

struct EntityTickTuple {
    data::Entity        &entity;
    data::AspectLife    &life;
    data::AspectPhysics &physics;
    data::Sprite        &sprite;

    EntityTickTuple(data::Entity &entity, data::AspectLife &life, data::AspectPhysics &physics, data::Sprite &sprite)
        : entity(entity), life(life), physics(physics), sprite(sprite) {}
};

struct EntityData {
    data::Entity          entity    {};
    data::AspectCombat    combat    {};
    data::AspectCollision collision {};
    data::AspectDialog    dialog    {};
    data::AspectLife      life      {};
    data::AspectPathfind  pathfind  {};
    data::AspectPhysics   physics   {};
    data::Sprite          sprite    {};
    data::AspectWarp      warp      {};

    AspectGhost           ghosts    {};
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