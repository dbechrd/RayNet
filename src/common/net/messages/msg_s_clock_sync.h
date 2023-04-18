#pragma once
#include "../../common.h"

struct Msg_S_ClockSync : public yojimbo::Message
{
    double serverTime{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, serverTime);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};