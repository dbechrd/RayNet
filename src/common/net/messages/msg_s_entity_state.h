#pragma once
#include "../../common.h"
#include "../entity_state.h"

struct Msg_S_EntityState : public yojimbo::Message
{
    int clientIdx{};
    EntityState entityState{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_int(stream, clientIdx, 0, yojimbo::MaxClients - 1);
        serialize_double(stream, entityState.serverTime);
        serialize_bits(stream, entityState.color.r, 8);
        serialize_bits(stream, entityState.color.g, 8);
        serialize_bits(stream, entityState.color.b, 8);
        serialize_bits(stream, entityState.color.a, 8);
        serialize_float(stream, entityState.size.x);
        serialize_float(stream, entityState.size.y);
        serialize_float(stream, entityState.speed);
        serialize_float(stream, entityState.velocity.x);
        serialize_float(stream, entityState.velocity.y);
        serialize_float(stream, entityState.position.x);
        serialize_float(stream, entityState.position.y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};