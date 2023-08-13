#pragma once
#include "../../common.h"

struct Msg_C_EntityInteractDialogOption : public yojimbo::Message
{
    uint32_t entity_id{};
    uint32_t dialog_id{};
    uint32_t option_id{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_uint32(stream, entity_id);
        serialize_uint32(stream, dialog_id);
        serialize_uint32(stream, option_id);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};