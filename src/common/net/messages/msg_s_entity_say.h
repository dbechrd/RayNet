#pragma once
#include "../../common.h"

struct Msg_S_EntitySay : public yojimbo::Message
{
    uint32_t entityId{};
    uint32_t messageLength{};
    char message[SV_MAX_ENTITY_SAY_MSG_LEN + 1]{};

    //enum SayDuration {
    //    SAY_SECONDS,
    //    SAY_UNTIL_RECV_NEXT,
    //    SAY_UNTIL_CLIENT_DISMISS
    //};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entityId);
        serialize_uint32(stream, messageLength);
        if (messageLength > SV_MAX_ENTITY_SAY_MSG_LEN) {
            return false;
        }
        serialize_string(stream, message, sizeof(message));
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};