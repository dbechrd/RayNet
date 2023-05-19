#pragma once
#include "../../common.h"

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    EntitySpawnEvent entitySpawnEvent{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, entitySpawnEvent.serverTime);
        serialize_uint32(stream, entitySpawnEvent.id);

        serialize_uint32(stream, (uint32_t&)entitySpawnEvent.entity.type);
        serialize_float(stream, entitySpawnEvent.entity.radius);
        serialize_float(stream, entitySpawnEvent.entity.drag);
        serialize_float(stream, entitySpawnEvent.entity.speed);
        serialize_float(stream, entitySpawnEvent.entity.velocity.x);
        serialize_float(stream, entitySpawnEvent.entity.velocity.y);
        serialize_float(stream, entitySpawnEvent.entity.position.x);
        serialize_float(stream, entitySpawnEvent.entity.position.y);
        serialize_uint32(stream, entitySpawnEvent.entity.sprite.spritesheetId);
        switch (entitySpawnEvent.entity.type) {
            case Entity_Player: {
                serialize_varint32(stream, entitySpawnEvent.entity.data.player.life.maxHealth);
                serialize_varint32(stream, entitySpawnEvent.entity.data.player.life.health);
                break;
            }
            case Entity_Bot: {
                serialize_varint32(stream, entitySpawnEvent.entity.data.bot.life.maxHealth);
                serialize_varint32(stream, entitySpawnEvent.entity.data.bot.life.health);
                break;
            }
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};