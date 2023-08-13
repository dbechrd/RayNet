#pragma once
#include "../../common.h"

struct Msg_C_EntityInteractDialogOption : public yojimbo::Message
{
    uint32_t entityId{};
    uint32_t optionId{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entityId);
        serialize_uint32(stream, optionId);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};