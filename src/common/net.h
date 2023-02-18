#pragma once
#include "common.h"
#include "yojimbo.h"

#define PROTOCOL_ID 0x42424242424242ULL
#define CLIENT_PORT 30000
#define SERVER_PORT 40000
#define FIXED_DT    (1.0f/60.0f)

struct MsgPlayerState : public yojimbo::Message
{
    // TODO(dlb): Should I type out all the serialized fields instead?
    Player player;

    MsgPlayerState()
    {
        memset(&player, 0, sizeof(player));
    }

    template <typename Stream> bool Serialize(Stream &stream)
    {
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

enum MsgType
{
    MSG_PLAYER_STATE,
    MSG_COUNT,
};

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_PLAYER_STATE, MsgPlayerState);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class NetAdapter : public yojimbo::Adapter
{
public:

    yojimbo::MessageFactory *CreateMessageFactory(yojimbo::Allocator &allocator)
    {
        return YOJIMBO_NEW(allocator, MsgFactory, allocator);
    }
};

extern NetAdapter netAdapter;

int yj_printf(const char *format, ...);
