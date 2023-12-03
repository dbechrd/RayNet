#pragma once
#include "../common.h"

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
    MSG_C_ENTITY_INTERACT_DIALOG_OPTION,
    MSG_C_INPUT_COMMANDS,
    MSG_C_TILE_INTERACT,

    MSG_S_CLOCK_SYNC,
    MSG_S_ENTITY_DESPAWN,
    MSG_S_ENTITY_SAY,
    MSG_S_ENTITY_SNAPSHOT,
    MSG_S_ENTITY_SPAWN,
    MSG_S_TILE_CHUNK,
    MSG_S_TILE_UPDATE,

    MSG_COUNT
};

extern const char *MsgTypeStr(MsgType type);

struct Msg_C_EntityInteract : public yojimbo::Message
{
    uint32_t entityId{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entityId);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_C_EntityInteractDialogOption : public yojimbo::Message
{
    uint32_t entity_id{};
    uint32_t dialog_id{};
    uint32_t option_id{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, dialog_id);
        serialize_uint32(stream, option_id);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_C_InputCommands : public yojimbo::Message
{
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        for (int i = 0; i < CL_SEND_INPUT_COUNT; i++) {
            InputCmd &cmd = cmdQueue[i];
            serialize_uint32(stream, cmd.seq);
            serialize_bool(stream, cmd.north);
            serialize_bool(stream, cmd.west);
            serialize_bool(stream, cmd.south);
            serialize_bool(stream, cmd.east);
            serialize_float(stream, cmd.facing.x);
            serialize_float(stream, cmd.facing.y);
            serialize_bool(stream, cmd.fire);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_C_TileInteract : public yojimbo::Message
{
    uint32_t map_id  {};
    uint32_t x       {};
    uint32_t y       {};
    bool     primary {};  // true = primary, false = secondary

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, map_id);
        serialize_bool(stream, primary);
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_ClockSync : public yojimbo::Message
{
    double serverTime{};
    uint32_t playerEntityId{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, serverTime);
        serialize_uint32(stream, playerEntityId);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_EntityDespawn : public yojimbo::Message
{
    uint32_t entityId{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entityId);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_EntitySay : public yojimbo::Message
{
    uint32_t entity_id{};
    uint32_t dialog_id{};
    char title[SV_MAX_ENTITY_SAY_TITLE_LEN + 1]{};
    char message[SV_MAX_ENTITY_SAY_MSG_LEN + 1]{};

    //enum SayDuration {
    //    SAY_SECONDS,
    //    SAY_UNTIL_RECV_NEXT,
    //    SAY_UNTIL_CLIENT_DISMISS
    //};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, dialog_id);
        serialize_string(stream, title, sizeof(title));
        serialize_string(stream, message, sizeof(message));

        //uint32_t message_len = MIN(message.size(), SV_MAX_ENTITY_SAY_MSG_LEN);
        //char message_buf[SV_MAX_ENTITY_SAY_MSG_LEN]{};
        //if (Stream::IsWriting) {
        //    strncpy(message_buf, message.c_str(), SV_MAX_ENTITY_SAY_MSG_LEN - 1);
        //}
        //serialize_uint32(stream, message_len);
        //if (message_len > SV_MAX_ENTITY_SAY_MSG_LEN) {
        //    return false;
        //}
        //serialize_bytes(stream, (uint8_t *)message_buf, message_len);
        //if (Stream::IsReading) {
        //    message = std::string{ message_buf, message_len };
        //}

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    double server_time {};

    // Entity
    uint32_t            entity_id  {};
    EntityType    type       {};  // doesn't change, but needed for switch statements in deserializer
    EntitySpecies spec       {};
    uint32_t            map_id     {};
    Vector3             position   {};
    bool                on_warp_cooldown {};

    // Collision
    //float       radius     {};  // when would this ever change? doesn't.. for now.

    // Life
    int         hp_max  {};
    int         hp      {};

    // Physics
    //float       speed      {};  // we don't need to know speed for ghosts
    Vector3     velocity   {};

    // Only for Entity_Player
    // TODO: Only send this to the player who actually owns this player entity,
    //       otherwise we're leaking info about other players' connections.
    int         client_idx  {};  // TODO: Populate this for lastprocessinputcmd
    uint32_t    last_processed_input_cmd {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, (uint32_t &)type);
        serialize_uint32(stream, (uint32_t &)spec);
        serialize_uint32(stream, map_id);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);
        serialize_float(stream, position.z);
        serialize_bool(stream, on_warp_cooldown);

        // Life
        serialize_varint32(stream, hp_max);
        if (hp_max) {
            serialize_varint32(stream, hp);
        }

        // Physics
        serialize_float(stream, velocity.x);
        serialize_float(stream, velocity.y);
        serialize_float(stream, velocity.z);

        // TODO: Also check if player is the clientIdx player. I.e. don't leak
        //       input info to all other players.
        if (type == ENTITY_PLAYER) {
            serialize_uint32(stream, last_processed_input_cmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    double        server_time {};

    // Entity
    uint32_t      entity_id   {};
    EntityType    type        {};
    EntitySpecies spec        {};
    char          name        [SV_MAX_ENTITY_NAME_LEN + 1]{};
    uint32_t      map_id      {};
    Vector3       position    {};
    // Collision
    float         radius      {};
    // Life
    int           hp_max      {};
    int           hp          {};
    // Physics
    float         drag        {};  // TODO: EntityType should imply this.. client should have prototypes
    float         speed       {};
    Vector3       velocity    {};
    // Sprite
    uint32_t      sprite_id   {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, (uint32_t&)type);
        serialize_uint32(stream, (uint32_t&)spec);
        serialize_string(stream, name, sizeof(name));
        serialize_uint32(stream, map_id);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);
        serialize_float(stream, position.z);

        // Collision
        serialize_float(stream, radius);

        // Life
        serialize_varint32(stream, hp_max);
        if (hp_max) {
            serialize_varint32(stream, hp);
        }

        // Physics
        serialize_float(stream, drag);
        serialize_float(stream, speed);
        serialize_float(stream, velocity.x);
        serialize_float(stream, velocity.y);
        serialize_float(stream, velocity.z);

        // Sprite
        serialize_uint32(stream, sprite_id);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_TileChunk : public yojimbo::BlockMessage
{
    uint32_t map_id {};
    uint32_t x      {};
    uint32_t y      {};
    uint32_t w      {};
    uint32_t h      {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, map_id);
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        serialize_uint32(stream, w);
        serialize_uint32(stream, h);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};
struct Msg_S_TileUpdate : public yojimbo::Message
{
    uint32_t map_id    {};
    uint32_t x         {};
    uint32_t y         {};
    uint32_t tile_id   {};
    uint32_t object_id {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, map_id);
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        serialize_uint32(stream, tile_id);
        serialize_uint32(stream, object_id);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

void InitClientServerConfig(yojimbo::ClientServerConfig &config);

YOJIMBO_MESSAGE_FACTORY_START(MsgFactory, MSG_COUNT);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_ENTITY_INTERACT,               Msg_C_EntityInteract);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_ENTITY_INTERACT_DIALOG_OPTION, Msg_C_EntityInteractDialogOption);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_INPUT_COMMANDS,                Msg_C_InputCommands);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_C_TILE_INTERACT,                 Msg_C_TileInteract);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_CLOCK_SYNC,                    Msg_S_ClockSync);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_DESPAWN,                Msg_S_EntityDespawn);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SAY,                    Msg_S_EntitySay);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SNAPSHOT,               Msg_S_EntitySnapshot);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_ENTITY_SPAWN,                  Msg_S_EntitySpawn);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_TILE_CHUNK,                    Msg_S_TileChunk);
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_TILE_UPDATE,                   Msg_S_TileUpdate);
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
