#pragma once
#include "../../common.h"

struct Msg_S_EntitySay : public yojimbo::Message
{
    uint32_t entityId{};
    std::string message{};

    //enum SayDuration {
    //    SAY_SECONDS,
    //    SAY_UNTIL_RECV_NEXT,
    //    SAY_UNTIL_CLIENT_DISMISS
    //};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        uint32_t messageLen = MIN(message.size(), SV_MAX_ENTITY_SAY_MSG_LEN);
        char messageBuf[SV_MAX_ENTITY_SAY_MSG_LEN]{};

        if (Stream::IsWriting) {
            strncpy(messageBuf, message.c_str(), SV_MAX_ENTITY_SAY_MSG_LEN - 1);
        }

        serialize_uint32(stream, entityId);
        serialize_uint32(stream, messageLen);
        if (messageLen > SV_MAX_ENTITY_SAY_MSG_LEN) {
            return false;
        }
        serialize_bytes(stream, (uint8_t *)messageBuf, messageLen);

        if (Stream::IsReading) {
            message = std::string{messageBuf, messageLen};
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};