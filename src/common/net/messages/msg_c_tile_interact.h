#pragma once
#include "../../common.h"

struct Msg_C_TileInteract : public yojimbo::Message
{
    uint32_t x{};
    uint32_t y{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};