#pragma once
#include "../../common.h"

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    double      serverTime {};

    // Entity
    uint32_t    entityId   {};
    EntityType  type       {};
    uint32_t    mapId      {};
    Vector2     position   {};

    // Collision
    float       radius     {};

    // Physics
    float       drag       {};  // TODO: EntityType should imply this.. client should have prototypes
    float       speed      {};
    Vector2     velocity   {};

    // Life
    int         maxHealth  {};
    int         health     {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, serverTime);

        // Entity
        serialize_uint32(stream, entityId);
        serialize_uint32(stream, (uint32_t&)type);
        serialize_uint32(stream, mapId);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);

        // Collision
        switch (type) {
            case Entity_NPC:
            case Entity_Player:
            case Entity_Projectile:
            {
                serialize_varint32(stream, radius);
                break;
            }
        }

        // Physics
        switch (type) {
            case Entity_NPC:
            case Entity_Player:
            case Entity_Projectile:
            {
                serialize_float(stream, drag);
                serialize_float(stream, speed);
                serialize_float(stream, velocity.x);
                serialize_float(stream, velocity.y);
            }
        }

        // Life
        switch (type) {
            case Entity_NPC:
            case Entity_Player:
            {
                serialize_varint32(stream, maxHealth);
                serialize_varint32(stream, health);
                break;
            }
        }


        // TODO: Client should know this implicitly based on entity.type
        //serialize_uint32(stream, spritesheetId);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};