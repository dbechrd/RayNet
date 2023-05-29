#pragma once
#include "../../common.h"

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