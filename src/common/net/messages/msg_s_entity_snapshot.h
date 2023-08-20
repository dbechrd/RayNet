#pragma once
#include "../../common.h"
#include "../../entity.h"

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    double            server_time {};

    // Entity
    uint32_t          entity_id  {};
    data::EntityType  type       {};  // doesn't change, but needed for switch statements in deserializer
    uint32_t          map_id     {};
    Vector2           position   {};

    // Collision
    //float       radius     {};  // when would this ever change? doesn't.. for now.

    // Life
    int         hp_max  {};
    int         hp      {};

    // Physics
    float       speed      {};
    Vector2     velocity   {};

    // Only for Entity_Player
    // TODO: Only send this to the player who actually owns this player entity,
    //       otherwise we're leaking info about other players' connections.
    int         client_idx  {};  // TODO: Populate this for lastprocessinputcmd
    uint32_t    last_processed_input_cmd {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, (uint32_t &)type);
        serialize_uint32(stream, map_id);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);

        // Physics
        switch (type) {
            case data::ENTITY_NPC:
            case data::ENTITY_PLAYER:
            case data::ENTITY_PROJECTILE:
            {
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

        // TODO: Also check if player is the clientIdx player. I.e. don't leak
        //       input info to all other players.
        if (type == data::ENTITY_PLAYER) {
            serialize_uint32(stream, last_processed_input_cmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};