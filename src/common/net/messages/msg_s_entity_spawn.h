#pragma once
#include "../../common.h"

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    double            server_time {};

    // Entity
    uint32_t            entity_id {};
    data::EntityType    type      {};
    data::EntitySpecies spec      {};
    char                name      [SV_MAX_ENTITY_NAME_LEN + 1]{};
    char                map_name  [SV_MAX_TILE_MAP_NAME_LEN + 1]{};
    Vector3             position  {};
    // Collision
    float               radius    {};
    // Life
    int                 hp_max    {};
    int                 hp        {};
    // Physics
    float               drag      {};  // TODO: EntityType should imply this.. client should have prototypes
    float               speed     {};
    Vector3             velocity  {};
    // Sprite
    char                sprite    [SV_MAX_SPRITE_NAME_LEN + 1]{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, (uint32_t&)type);
        serialize_uint32(stream, (uint32_t&)spec);
        serialize_string(stream, name, sizeof(name));
        serialize_string(stream, map_name, sizeof(map_name));
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);
        serialize_float(stream, position.z);

        // Collision
        serialize_float(stream, radius);

        // Life
        serialize_varint32(stream, hp_max);
        if (hp_max) {
            serialize_varint32(stream, hp);
        }

        // Physics
        serialize_float(stream, drag);
        serialize_float(stream, speed);
        serialize_float(stream, velocity.x);
        serialize_float(stream, velocity.y);
        serialize_float(stream, velocity.z);

        // Sprite
        serialize_string(stream, sprite, sizeof(sprite));

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};