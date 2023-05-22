#pragma once
#include "common.h"

struct Tilemap;

struct AspectCollision {
    float radius;    // collision
    bool colliding;  // not sync'd, local flag for debugging colliders
};

struct AspectDialog {
    // Kind of an intrusive index of sorts.. used by dialog system to determine
    // if entity has a currently active dialog. Probably dialog system should
    // just maintain a map <entityid, dialogid> itself. How to handle dead
    // entities cleaning up their dialogs? I suppose the map could enable that.
    uint32_t latestDialog;
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

enum EntityType {
    Entity_None,
    Entity_Player,
    Entity_NPC,
    Entity_Projectile,
    Entity_Count,
};

struct Entity {
    EntityType type;
    double spawnedAt;
    double despawnedAt;
    Vector2 position;

    // TODO: Separate this out into its own array?
    uint32_t freelist_next;

    static void Tick(Tilemap &map, uint32_t entityId, double dt);
    static Rectangle GetRect(Tilemap &map, uint32_t entityId);
    static Vector2 TopCenter(Tilemap &map, uint32_t entityId);
    static void DrawHoverInfo(Tilemap &map, uint32_t entityId);
    static void Draw(Tilemap &map, uint32_t entityId, double now);
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