#pragma once
#include "../../common.h"
#include "../entity_snapshot.h"

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    EntitySnapshot entitySnapshot{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entitySnapshot.id);
        uint32_t type = (uint32_t)entitySnapshot.type;
        serialize_uint32(stream, type);
        entitySnapshot.type = (EntityType)type;
        serialize_double(stream, entitySnapshot.serverTime);
        serialize_bits(stream, entitySnapshot.color.r, 8);
        serialize_bits(stream, entitySnapshot.color.g, 8);
        serialize_bits(stream, entitySnapshot.color.b, 8);
        serialize_bits(stream, entitySnapshot.color.a, 8);
        serialize_float(stream, entitySnapshot.size.x);
        serialize_float(stream, entitySnapshot.size.y);
        serialize_float(stream, entitySnapshot.speed);
        serialize_float(stream, entitySnapshot.velocity.x);
        serialize_float(stream, entitySnapshot.velocity.y);
        serialize_float(stream, entitySnapshot.position.x);
        serialize_float(stream, entitySnapshot.position.y);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};