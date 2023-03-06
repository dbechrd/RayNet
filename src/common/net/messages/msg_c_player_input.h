#pragma once
#include "../../common.h"
#include "../../input_command.h"

struct Msg_C_PlayerInput : public yojimbo::Message
{
    InputCommand playerInput{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_bool(stream, playerInput.north);
        serialize_bool(stream, playerInput.west);
        serialize_bool(stream, playerInput.south);
        serialize_bool(stream, playerInput.east);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};
