#pragma once
#include "../../common.h"

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    EntitySpawnEvent entitySpawnEvent{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entitySpawnEvent.id);
        uint32_t type = (uint32_t)entitySpawnEvent.type;
        serialize_uint32(stream, type);
        entitySpawnEvent.type = (EntityType)type;
        serialize_double(stream, entitySpawnEvent.serverTime);
        serialize_uint32(stream, entitySpawnEvent.lastProcessedInputCmd);
        serialize_bits(stream, entitySpawnEvent.color.r, 8);
        serialize_bits(stream, entitySpawnEvent.color.g, 8);
        serialize_bits(stream, entitySpawnEvent.color.b, 8);
        serialize_bits(stream, entitySpawnEvent.color.a, 8);
        serialize_float(stream, entitySpawnEvent.size.x);
        serialize_float(stream, entitySpawnEvent.size.y);
        serialize_float(stream, entitySpawnEvent.radius);
        serialize_float(stream, entitySpawnEvent.drag);
        serialize_float(stream, entitySpawnEvent.speed);
        serialize_float(stream, entitySpawnEvent.velocity.x);
        serialize_float(stream, entitySpawnEvent.velocity.y);
        serialize_float(stream, entitySpawnEvent.position.x);
        serialize_float(stream, entitySpawnEvent.position.y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};