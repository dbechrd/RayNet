#pragma once
#include "../../common.h"

struct Msg_S_ClockSync : public yojimbo::Message
{
    double serverTime;

    Msg_S_ClockSync() : serverTime(0) {}

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, serverTime);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};