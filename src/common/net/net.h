#pragma once
#include "../common.h"
#include "../input_command.h"

#define serialize_uint8( stream, value ) serialize_bits( stream, value, 8 );
#define serialize_uint16( stream, value ) serialize_bits( stream, value, 16 );

enum ChannelType {
    CHANNEL_U_INPUT_COMMANDS,
    CHANNEL_U_ENTITY_SNAPSHOT,

    CHANNEL_R_CLOCK_SYNC,
    CHANNEL_R_ENTITY_EVENT,
    CHANNEL_R_TILE_EVENT,
    CHANNEL_R_GLOBAL_EVENT,

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
    MSG_S_TITLE_SHOW,

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
    uint16_t dialog_id{};
    uint16_t option_id{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entity_id);
        serialize_uint16(stream, dialog_id);
        serialize_uint16(stream, option_id);
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
            serialize_uint8(stream, cmd.seq);
            serialize_uint8(stream, cmd.facing);
            serialize_bool(stream, cmd.north);
            serialize_bool(stream, cmd.west);
            serialize_bool(stream, cmd.south);
            serialize_bool(stream, cmd.east);
            serialize_bool(stream, cmd.fire);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_C_TileInteract : public yojimbo::Message
{
    uint16_t map_id  {};
    uint16_t x       {};
    uint16_t y       {};
    bool     primary {};  // true = primary, false = secondary

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint16(stream, map_id);
        serialize_uint16(stream, x);
        serialize_uint16(stream, y);
        serialize_bool(stream, primary);
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
    uint16_t dialog_id{};
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
        serialize_uint16(stream, dialog_id);
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
    uint32_t        entity_id  {};
    Entity::Type    type       {};  // doesn't change, but needed for switch statements in deserializer
    Entity::Species spec       {};
    uint16_t        map_id     {};
    Vector3         position   {};
    bool            on_warp_cooldown {};

    // Collision
    //float       radius     {};  // when would this ever change? doesn't.. for now.

    // Life
    int           hp_max  {};
    int           hp      {};

    // Physics
    //float       speed      {};  // we don't need to know speed for ghosts
    Vector3       velocity   {};

    // Only sent to the player who owns this player entity.
    uint8_t       last_processed_input_cmd {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint8(stream, (uint8_t &)type);
        serialize_uint8(stream, (uint8_t &)spec);
        serialize_uint16(stream, map_id);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);
        serialize_float(stream, position.z);

        // Physics
        serialize_float(stream, velocity.x);
        serialize_float(stream, velocity.y);
        serialize_float(stream, velocity.z);

        // Life
        if (type == Entity::TYP_PLAYER || type == Entity::TYP_NPC) {
            serialize_varint32(stream, hp_max);
            if (hp_max) {
                serialize_varint32(stream, hp);
            }
        }

        if (type == Entity::TYP_PLAYER) {
            serialize_bool(stream, on_warp_cooldown);
            serialize_uint8(stream, last_processed_input_cmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_EntitySpawn : public yojimbo::Message
{
    double        server_time {};

    // Entity
    uint32_t        entity_id   {};
    Entity::Type    type        {};
    Entity::Species spec        {};
    char            name        [SV_MAX_ENTITY_NAME_LEN + 1]{};
    uint16_t        map_id      {};
    Vector3         position    {};
    // Collision
    float           radius      {};
    // Life
    int             hp_max      {};
    int             hp          {};
    // Physics
    float           drag        {};  // TODO: EntityType should imply this.. client should have prototypes
    float           speed       {};
    Vector3         velocity    {};
    // Sprite
    uint16_t        sprite_id   {};

    // Only sent to the player who owns this player entity.
    uint8_t       last_processed_input_cmd {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, server_time);

        // Entity
        serialize_uint32(stream, entity_id);
        serialize_uint8(stream, (uint8_t&)type);
        serialize_uint8(stream, (uint8_t&)spec);
        serialize_string(stream, name, sizeof(name));
        serialize_uint16(stream, map_id);
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

        // Player
        if (type == Entity::TYP_PLAYER) {
            serialize_uint8(stream, last_processed_input_cmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_TileChunk : public yojimbo::BlockMessage
{
    uint16_t map_id    {};
    uint16_t map_w     {};
    uint16_t map_h     {};
    uint16_t x         {};
    uint16_t y         {};
    uint16_t w         {};
    uint16_t h         {};
    uint32_t beeg_size {};
    uint32_t smol_size {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint16(stream, map_id);
        serialize_uint16(stream, map_w);
        serialize_uint16(stream, map_h);
        serialize_uint16(stream, x);
        serialize_uint16(stream, y);
        serialize_uint16(stream, w);
        serialize_uint16(stream, h);
        serialize_uint32(stream, smol_size);
        serialize_uint32(stream, beeg_size);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_TileUpdate : public yojimbo::Message
{
    uint16_t map_id    {};
    uint16_t x         {};
    uint16_t y         {};
    uint16_t tile_ids  [TILE_LAYER_COUNT]{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint16(stream, map_id);
        serialize_uint16(stream, x);
        serialize_uint16(stream, y);
        for (int layer = 0; layer < TILE_LAYER_COUNT; layer++) {
            serialize_uint16(stream, tile_ids[layer]);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct Msg_S_TitleShow : public yojimbo::Message
{
    char text [SV_MAX_TITLE_LEN + 1]{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_string(stream, text, sizeof(text));
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
YOJIMBO_DECLARE_MESSAGE_TYPE(MSG_S_TITLE_SHOW,                    Msg_S_TitleShow);
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
