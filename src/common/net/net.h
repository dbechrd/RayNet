#pragma once
#include "../common.h"
#include "messages/msg_c_input_commands.h"
#include "messages/msg_s_clock_sync.h"
#include "messages/msg_s_entity_snapshot.h"

enum ChannelType {
    CHANNEL_U_CLOCK_SYNC,
    CHANNEL_U_INPUT_COMMANDS,
    CHANNEL_U_ENTITY_SNAPSHOT,
    CHANNEL_COUNT
};

enum MsgType
{
    MSG_C_INPUT_COMMANDS,

    MSG_S_CLOCK_SYNC,
    MSG_S_ENTITY_SNAPSHOT,

    MSG_COUNT
};

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_INPUT_COMMANDS, Msg_C_InputCommands);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_CLOCK_SYNC, Msg_S_ClockSync);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SNAPSHOT, Msg_S_EntitySnapshot);
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
