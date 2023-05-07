#pragma once
#include "common.h"

struct Tilemap;

struct WangMap {
    Image image{};
    Texture texture{};  // each pixel is an index into tileDefs

    ~WangMap(void) {
        UnloadImage(image);
        UnloadTexture(texture);
    }
};

struct WangTileset {
    const PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    stbhw_tileset tileset;

    std::vector<Texture> hTextures{};
    std::vector<Texture> vTextures{};

    Err GenerateTemplate(const char *filename);
    void GenerateHTileTexture(Tilemap &map, int hTex);
    void GenerateVTileTexture(Tilemap &map, int vTex);
    Err Load(Tilemap &map, const char *filename);
    void Unload(void);
    Err GenerateMap(uint32_t w, uint32_t h, WangMap &wangMap);
};