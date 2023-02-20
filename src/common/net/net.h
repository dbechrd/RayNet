#pragma once
#include "../common.h"
#include "messages/msg_c_player_input.h"
#include "messages/msg_s_clock_sync.h"
#include "messages/msg_s_entity_state.h"

#define PROTOCOL_ID           0x42424242424242ULL

#define SERVER_PORT           40000
#define SERVER_TICK_DT        (1.0/20.0)

#define CLIENT_PORT           30000
#define CLIENT_SEND_INPUT_DT  (1.0/60.0)
#define CLIENT_SNAPSHOT_COUNT 16

enum ChannelType {
    CHANNEL_U_CLOCK_SYNC,
    CHANNEL_U_PLAYER_INPUT,
    CHANNEL_U_PLAYER_STATE,
    CHANNEL_COUNT
};

enum MsgType
{
    MSG_C_PLAYER_INPUT,

    MSG_S_CLOCK_SYNC,
    MSG_S_ENTITY_STATE,

    MSG_COUNT
};

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_PLAYER_INPUT, Msg_C_PlayerInput);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_CLOCK_SYNC, Msg_S_ClockSync);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_STATE, Msg_S_EntityState);
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
