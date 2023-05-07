#include "wang.h"
#include "shared_lib.h"

Err WangTileset::GenerateTemplate(const char *filename)
{
    Err err = RN_SUCCESS;

    stbhw_config config{};
    config.is_corner = 0;
    config.short_side_len = 8;
    config.num_color[0] = 1;
    config.num_color[1] = 1;
    config.num_color[2] = 1;
    config.num_color[3] = 1;
    config.num_color[4] = 1;
    config.num_color[5] = 1;
    config.num_vary_x = 1;
    config.num_vary_y = 1;

    int w{};
    int h{};
    stbhw_get_template_size(&config, &w, &h);

    unsigned char *pixels = (unsigned char *)calloc((size_t)3 * w * h, 1);
    do {
        if (!pixels) {
            printf("[wang_tileset] Failed allocate wang template pixel buffer.\n");
            err = RN_BAD_ALLOC; break;
        }

        if (!stbhw_make_template(&config, pixels, w, h, w*3)) {
            printf("[wang_tileset] Failed to generate wang template. %s\n", stbhw_get_last_error());
            err = RN_BAD_ALLOC; break;
        }

        if (!ExportImage({ pixels, w, h, 1, format }, filename)) {
            printf("[wang_tileset] Failed to export wang template.\n");
            err = RN_BAD_FILE_WRITE; break;
        }
    } while(0);

    free(pixels);
    return err;
}

void WangTileset::GenerateHTileTexture(Tilemap &map, int hTex)
{
    Image img{};
    img.width = tileset.short_side_len * 2;
    img.height = tileset.short_side_len;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    img.data = calloc((size_t)img.width * img.height, 3);
    for (int i = 0; i < img.width * img.height; i++) {
        uint8_t tile = tileset.h_tiles[hTex]->pixels[i * 3];
        Color color = map.TileDefAvgColor(tile);
        ((uint8_t *)img.data)[i*3+0] = color.r;
        ((uint8_t *)img.data)[i*3+1] = color.g;
        ((uint8_t *)img.data)[i*3+2] = color.b;
    }
    UnloadTexture(hTextures[hTex]);
    hTextures[hTex] = LoadTextureFromImage(img);
    free(img.data);
}

void WangTileset::GenerateVTileTexture(Tilemap &map, int vTex)
{
    Image img{};
    img.width = tileset.short_side_len;
    img.height = tileset.short_side_len * 2;
    img.mipmaps = 1;
    img.format = format;
    img.data = calloc((size_t)img.width * img.height, 3);
    for (int i = 0; i < img.width * img.height; i++) {
        uint8_t tile = tileset.v_tiles[vTex]->pixels[i * 3];
        Color color = map.TileDefAvgColor(tile);
        ((uint8_t *)img.data)[i*3+0] = color.r;
        ((uint8_t *)img.data)[i*3+1] = color.g;
        ((uint8_t *)img.data)[i*3+2] = color.b;
    }
    UnloadTexture(vTextures[vTex]);
    vTextures[vTex] = LoadTextureFromImage(img);
    free(img.data);
}

Err WangTileset::Load(Tilemap &map, const char *filename)
{
    const char *wangTilesetPath = "resources/wang/tileset.png";
    Image wangTilesetImg = LoadImage(wangTilesetPath);
    if (wangTilesetImg.width) {
        ImageFormat(&wangTilesetImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        if (!stbhw_build_tileset_from_image(&tileset, (unsigned char *)wangTilesetImg.data, wangTilesetImg.width*3, wangTilesetImg.width, wangTilesetImg.height)) {
            printf("[wang_tileset] Failed to build tileset from image %s. %s\n", wangTilesetPath, stbhw_get_last_error());
            return RN_BAD_FILE_READ;
        }
        UnloadImage(wangTilesetImg);

        assert(tileset.num_h_tiles);
        assert(tileset.num_v_tiles);

        if (!tileset.num_h_tiles || !tileset.num_v_tiles) {
            printf("[wang_tileset] Tileset must have at least 1 horizontal and vertical tile each. hor: %d ver: %d\n", tileset.num_h_tiles, tileset.num_v_tiles);
            return RN_BAD_FILE_READ;
        }

        hTextures.resize(tileset.num_h_tiles);
        vTextures.resize(tileset.num_v_tiles);

        const int len = tileset.short_side_len;
        const int len2 = tileset.short_side_len * 2;
        for (int i = 0; i < tileset.num_h_tiles; i++) {
            GenerateHTileTexture(map, i);
        }
        for (int i = 0; i < tileset.num_v_tiles; i++) {
            GenerateVTileTexture(map, i);
        }
    }

    return RN_SUCCESS;
}

void WangTileset::Unload(void)
{
    for (Texture &tex : hTextures) UnloadTexture(tex);
    for (Texture &tex : vTextures) UnloadTexture(tex);
    stbhw_free_tileset(&tileset);
}

Err WangTileset::GenerateMap(uint32_t w, uint32_t h, WangMap &wangMap)
{
    Err err = RN_SUCCESS;

    if (!tileset.num_h_tiles || !tileset.num_v_tiles) {
        printf("[wang_tileset] Cannot generate map without tiles. Did you load the tileset first?\n");
        return RN_INVALID_SIZE;
    }

    UnloadImage(wangMap.image);
    UnloadTexture(wangMap.texture);

    unsigned char *pixels = (unsigned char *)calloc((size_t)3 * w * h, 1);
    Image img{};
    do {
        if (!pixels) {
            err = RN_BAD_ALLOC; break;
        }

        if (!stbhw_generate_image(&tileset, 0, pixels, w*3, w, h)) {
            free(pixels);
            printf("[wang_tileset] Failed to generate map from tileset. %s\n", stbhw_get_last_error());
            err = RN_BAD_ALLOC; break;
        }

        img.width = w;
        img.height = h;
        img.format = format;
        img.data = pixels;
        img.mipmaps = 1;
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

        wangMap.image = img;
        wangMap.texture = LoadTextureFromImage(wangMap.image);
        if (!wangMap.texture.width) {
            UnloadImage(img);
            printf("[wang_tileset] Failed to create texture from wang map image.\n");
            err = RN_BAD_ALLOC; break;
        }
    } while (0);

    return err;
}