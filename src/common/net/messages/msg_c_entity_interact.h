#pragma once
#include "../../common.h"

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