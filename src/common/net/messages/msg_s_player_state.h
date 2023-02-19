#pragma once
#include "../../common.h"
#include "../../player.h"

struct Msg_S_PlayerState : public yojimbo::Message
{
    // TODO(dlb): Should I type out all the serialized fields instead?
    int clientIdx;
    Player player;

    Msg_S_PlayerState()
    {
        clientIdx = 0;
        memset(&player, 0, sizeof(player));
    }

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_int(stream, clientIdx, 0, yojimbo::MaxClients - 1);
        serialize_bytes(stream, (uint8_t *)&player.color, sizeof(player.color));
        serialize_float(stream, player.size.x);
        serialize_float(stream, player.size.y);
        serialize_float(stream, player.speed);
        serialize_float(stream, player.velocity.x);
        serialize_float(stream, player.velocity.y);
        serialize_float(stream, player.position.x);
        serialize_float(stream, player.position.y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};