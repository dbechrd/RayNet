#pragma once
#include "../../common.h"
#include "../../data.h"

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    double server_time {};

    // Entity
    uint32_t            entity_id  {};
    data::EntityType    type       {};  // doesn't change, but needed for switch statements in deserializer
    data::EntitySpecies spec       {};
    char                map_name   [SV_MAX_TILE_MAP_NAME_LEN + 1]{};
    Vector3             position   {};

    // Collision
    //float       radius     {};  // when would this ever change? doesn't.. for now.

    // Life
    int         hp_max  {};
    int         hp      {};

    // Physics
    //float       speed      {};  // we don't need to know speed for ghosts
    Vector3     velocity   {};

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
        serialize_uint32(stream, (uint32_t &)spec);
        serialize_string(stream, map_name, sizeof(map_name));
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);
        serialize_float(stream, position.z);

        // Life
        serialize_varint32(stream, hp_max);
        if (hp_max) {
            serialize_varint32(stream, hp);
        }

        // Physics
        serialize_float(stream, velocity.x);
        serialize_float(stream, velocity.y);
        serialize_float(stream, velocity.z);

        // TODO: Also check if player is the clientIdx player. I.e. don't leak
        //       input info to all other players.
        if (type == data::ENTITY_PLAYER) {
            serialize_uint32(stream, last_processed_input_cmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};