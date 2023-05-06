#pragma once
#include "../../common.h"
#include "../entity_snapshot.h"

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    EntitySnapshot entitySnapshot{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, entitySnapshot.serverTime);
        serialize_uint32(stream, entitySnapshot.id);

        serialize_uint32(stream, (uint32_t &)entitySnapshot.entity.type);
        serialize_float(stream, entitySnapshot.entity.velocity.x);
        serialize_float(stream, entitySnapshot.entity.velocity.y);
        serialize_float(stream, entitySnapshot.entity.position.x);
        serialize_float(stream, entitySnapshot.entity.position.y);
        switch (entitySnapshot.entity.type) {
            case Entity_Player: {
                serialize_uint32(stream, entitySnapshot.lastProcessedInputCmd);
                serialize_varint32(stream, entitySnapshot.entity.data.player.life.maxHealth);
                serialize_varint32(stream, entitySnapshot.entity.data.player.life.health);
                break;
            }
            case Entity_Bot: {
                serialize_varint32(stream, entitySnapshot.entity.data.bot.life.maxHealth);
                serialize_varint32(stream, entitySnapshot.entity.data.bot.life.health);
                break;
            }
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};