#pragma once
#include "common.h"

struct Tilemap;

struct WangMap {
    Image image{};
    Texture indexed{};  // each pixel is an index into tileDefs
    Texture colorized{};  // each pixel is the pretty tileDef color (i.e. a minimap)

    ~WangMap(void) {
        UnloadImage(image);
        UnloadTexture(indexed);
        UnloadTexture(colorized);
    }
};

struct WangTileset {
    const PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    stbhw_tileset ts;
    Texture tsColorized{};

    Err GenerateTemplate(const char *filename);
    Texture GenerateColorizedTexture(Image &image, Tilemap &map);
    Err Load(Tilemap &map, const char *filename);
    void Unload(void);
    Err GenerateMap(uint32_t w, uint32_t h, Tilemap &map, WangMap &wangMap);
};