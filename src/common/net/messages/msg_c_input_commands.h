#pragma once
#include "../../common.h"
#include "../../input_command.h"

struct Msg_C_InputCommands : public yojimbo::Message
{
    InputCommandQueue cmdQueue{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        for (int i = 0; i < CLIENT_SEND_INPUT_COUNT; i++) {
            InputCommand &cmd = cmdQueue.data[i];
            serialize_uint32(stream, cmd.seq);
            serialize_bool(stream, cmd.north);
            serialize_bool(stream, cmd.west);
            serialize_bool(stream, cmd.south);
            serialize_bool(stream, cmd.east);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};
