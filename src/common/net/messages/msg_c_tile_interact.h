#pragma once
#include "../../common.h"

struct Msg_C_TileInteract : public yojimbo::Message
{
    char     map_id [SV_MAX_TILE_MAP_NAME_LEN]{};
    uint32_t x      {};
    uint32_t y      {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_string(stream, map_id, sizeof(map_id));
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};