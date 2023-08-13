#pragma once
#include "../../common.h"

struct Msg_S_TileChunk : public yojimbo::Message
{
    //std::string mapName{};
    uint32_t map_id{};
    uint32_t x{};
    uint32_t y{};
    uint8_t tileDefs[SV_TILE_CHUNK_WIDTH * SV_TILE_CHUNK_WIDTH]{};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        //char mapNameBuf[PATH_LEN_MAX + 1]{};
        //strcpy(mapNameBuf, mapName.c_str());
        //serialize_string(stream, mapNameBuf, PATH_LEN_MAX);
        //mapName = mapNameBuf;
        serialize_uint32(stream, map_id);
        serialize_uint32(stream, x);
        serialize_uint32(stream, y);
        serialize_bytes(stream, tileDefs, sizeof(tileDefs));
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};