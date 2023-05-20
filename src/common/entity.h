#pragma once
#include "common.h"
#include "aspect_sprite.h"

struct Tilemap;

struct AspectPhysics {
    float drag;
    float speed;
    Vector2 forceAccum;
    Vector2 velocity;
};

struct AspectCollision {
    float radius;    // collision
    bool colliding;  // not sync'd, local flag for debugging colliders
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

    //template <typename Stream> bool Serialize(Stream &stream)

    // Kind of an intrusive index of sorts.. used by dialog system to determine
    // if entity has a currently active dialog. Probably dialog system should
    // just maintain a map <entityid, dialogid> itself. How to handle dead
    // entities cleaning up their dialogs? I suppose the map could enable that.
    uint32_t latestDialog;

    //uint8_t custom_data[1024];
    //union {
    //    EntityPlayer player;
    //    EntityNPC npc;
    //    EntityProjectile projectile;
    //};

    uint32_t freelist_next;

    static void ApplyForce(Tilemap &map, uint32_t entityId, Vector2 force);
    static void Tick(Tilemap &map, uint32_t entityId, double dt);
    static Rectangle GetRect(Tilemap &map, uint32_t entityId);
    static Vector2 TopCenter(Tilemap &map, uint32_t entityId);
    //static AspectLife *GetLife(Tilemap &map, uint32_t entityId, void);
    static void DrawHoverInfo(Tilemap &map, uint32_t entityId);
    static void Draw(Tilemap &map, uint32_t entityId, double now);
};