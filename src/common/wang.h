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

    std::string filename{};
    stbhw_tileset ts{};
    Texture thumbnail{};

    //std::vector<Tilemap> hWangTilemaps{};
    //std::vector<Tilemap> vWangTilemaps{};

    ~WangTileset(void);

    Err Load(Tilemap &map, std::string path);

    Err GenerateTemplate(std::string path);
    Texture GenerateColorizedTexture(Image &image, Tilemap &map);
    Err GenerateMap(uint32_t w, uint32_t h, Tilemap &map, WangMap &wangMap);

private:
    void Unload(void);
};