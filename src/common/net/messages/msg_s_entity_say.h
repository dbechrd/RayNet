#pragma once
#include "../../common.h"

struct Msg_S_EntitySay : public yojimbo::Message
{
    uint32_t entity_id{};
    uint32_t dialog_id{};
    std::string message{};

    //enum SayDuration {
    //    SAY_SECONDS,
    //    SAY_UNTIL_RECV_NEXT,
    //    SAY_UNTIL_CLIENT_DISMISS
    //};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        uint32_t message_len = MIN(message.size(), SV_MAX_ENTITY_SAY_MSG_LEN);
        char message_buf[SV_MAX_ENTITY_SAY_MSG_LEN]{};
        if (Stream::IsWriting) {
            strncpy(message_buf, message.c_str(), SV_MAX_ENTITY_SAY_MSG_LEN - 1);
        }

        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, dialog_id);
        serialize_uint32(stream, message_len);

        if (message_len > SV_MAX_ENTITY_SAY_MSG_LEN) {
            return false;
        }
        serialize_bytes(stream, (uint8_t *)message_buf, message_len);
        if (Stream::IsReading) {
            message = std::string{ message_buf, message_len };
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};