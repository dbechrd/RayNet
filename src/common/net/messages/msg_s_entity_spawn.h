#pragma once
#include "../../common.h"

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    double            server_time {};

    // Entity
    uint32_t          entity_id  {};
    data::EntityType  type       {};
    uint32_t          map_id     {};
    Vector2           position   {};

    // Collision
    float       radius   {};

    // Physics
    float       drag     {};  // TODO: EntityType should imply this.. client should have prototypes
    float       speed    {};
    Vector2     velocity {};

    // Life
    int         hp_max   {};
    int         hp       {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, (uint32_t&)type);
        serialize_uint32(stream, map_id);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);

        // Collision
        switch (type) {
            case data::ENTITY_NPC:
            case data::ENTITY_PLAYER:
            case data::ENTITY_PROJECTILE:
            {
                serialize_varint32(stream, radius);
                break;
            }
        }

        // Physics
        switch (type) {
            case data::ENTITY_NPC:
            case data::ENTITY_PLAYER:
            case data::ENTITY_PROJECTILE:
            {
                serialize_float(stream, drag);
                serialize_float(stream, speed);
                serialize_float(stream, velocity.x);
                serialize_float(stream, velocity.y);
            }
        }

        // Life
        switch (type) {
            case data::ENTITY_NPC:
            case data::ENTITY_PLAYER:
            {
                serialize_varint32(stream, hp_max);
                serialize_varint32(stream, hp);
                break;
            }
        }


        // TODO: Client should know this implicitly based on entity.type
        //serialize_uint32(stream, spritesheetId);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};