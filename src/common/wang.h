#pragma once
#include "common.h"

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
    static const PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    std::string filename{};
    stbhw_tileset ts{};
    Texture thumbnail{};

    //std::vector<Tilemap> hWangTilemaps{};
    //std::vector<Tilemap> vWangTilemaps{};

    ~WangTileset(void);

    Err Load(const std::string &path);

    static Err GenerateTemplate(const std::string &path);
    Texture GenerateColorizedTexture(Image &image);
    Err GenerateMap(uint32_t w, uint32_t h, WangMap &wangMap);

private:
    void Unload(void);
};