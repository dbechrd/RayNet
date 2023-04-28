#pragma once
#include "../common.h"
#include "messages/msg_c_entity_interact.h"
#include "messages/msg_c_input_commands.h"
#include "messages/msg_c_tile_interact.h"
#include "messages/msg_s_clock_sync.h"
#include "messages/msg_s_entity_despawn.h"
#include "messages/msg_s_entity_say.h"
#include "messages/msg_s_entity_snapshot.h"
#include "messages/msg_s_entity_spawn.h"

enum ChannelType {
    CHANNEL_U_INPUT_COMMANDS,
    CHANNEL_U_ENTITY_SNAPSHOT,

    CHANNEL_R_CLOCK_SYNC,
    CHANNEL_R_ENTITY_EVENT,
    CHANNEL_R_TILE_EVENT,

    CHANNEL_COUNT
};

enum MsgType
{
    MSG_C_ENTITY_INTERACT,
    MSG_C_INPUT_COMMANDS,
    MSG_C_TILE_INTERACT,

    MSG_S_CLOCK_SYNC,
    MSG_S_ENTITY_DESPAWN,
    MSG_S_ENTITY_SAY,
    MSG_S_ENTITY_SNAPSHOT,
    MSG_S_ENTITY_SPAWN,

    MSG_COUNT
};

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_ENTITY_INTERACT, Msg_C_EntityInteract);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_INPUT_COMMANDS,  Msg_C_InputCommands);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_TILE_INTERACT,   Msg_C_TileInteract);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_CLOCK_SYNC,      Msg_S_ClockSync);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_DESPAWN,  Msg_S_EntityDespawn);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SAY,      Msg_S_EntitySay);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SNAPSHOT, Msg_S_EntitySnapshot);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SPAWN,    Msg_S_EntitySpawn);
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
