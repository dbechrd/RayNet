#pragma once
#include "../common.h"
#include "messages/msg_c_player_input.h"
#include "messages/msg_s_player_state.h"

#define PROTOCOL_ID    0x42424242424242ULL
#define CLIENT_PORT    30000
#define SERVER_PORT    40000
#define FIXED_DT       (1.0/60.0)
#define SERVER_TICK_DT (1.0/10.0)

enum MsgType
{
    MSG_C_PLAYER_INPUT,
    MSG_S_PLAYER_STATE,
    MSG_COUNT,
};

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_PLAYER_INPUT, Msg_C_PlayerInput);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_PLAYER_STATE, Msg_S_PlayerState);
YOJIMBO_MESSAGE_FACTORY_FINISH();

class NetAdapter : public yojimbo::Adapter
{
public:

    yojimbo::MessageFactory *CreateMessageFactory(yojimbo::Allocator &allocator)
    {
        return YOJIMBO_NEW(allocator, MsgFactory, allocator);
    }
};

int yj_printf(const char *format, ...);
