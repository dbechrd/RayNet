#include "wang.h"
#include "tilemap.h"

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
    config.num_vary_x = 2;
    config.num_vary_y = 2;

    int w{};
    int h{};
    stbhw_get_template_size(&config, &w, &h);

    unsigned char *pixels = (unsigned char *)calloc((size_t)3 * w * h, 1);
    do {
        if (!pixels) {
            printf("[wang_tileset] Failed allocate wang template pixel buffer.\n");
            err = RN_BAD_ALLOC; break;
        }

        Image img{};
        img.data = pixels;
        img.width = w;
        img.height = h;
        img.mipmaps = 1;
        img.format = format;
        if (!stbhw_make_template(&config, img)) {
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

Texture WangTileset::GenerateColorizedTexture(Image &image, Tilemap &map)
{
    Image img{};
    img.width = image.width;
    img.height = image.height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    size_t pixelsLen = (size_t)img.width * img.height * 3;
    uint8_t *pixels = (uint8_t *)calloc(pixelsLen, 1);
    img.data = pixels;

    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            // Read indexed color from tileset image
            Color tileIndexColor = GetImageColor(image, x, y);
            uint8_t tile = tileIndexColor.r < (map.tileDefs.size()) ? tileIndexColor.r : 0;
#if 0
            Color color = map.TileDefAvgColor(tile);
#else
            Color color = tileIndexColor;
            color.r = color.r ^ (tile * 55);
            color.g = color.g ^ (tile * 55);
            color.b = color.b ^ (tile * 55);
#endif
            // Write colorized pixel to output
            size_t pixelIdx = (y * img.width + x) * 3;
            assert(pixelIdx < pixelsLen);
            uint8_t *pixel = &pixels[pixelIdx];
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
        }
    }

    Texture colorized = LoadTextureFromImage(img);
    free(img.data);
    return colorized;
}

#if 0
void WangTileset::RegenerateColorizedTileset(void)
{
    Image colorized = ImageCopy(wangTilesetImg);
    for (int i = 0; i < ts.num_h_tiles; i++) {
        stbhw_tile *wangTile = ts.h_tiles[i];
        for (int y = wangTile->y; y < ts.short_side_len; y++) {
            for (int x = wangTile->x; x < ts.short_side_len*2; x++) {
                Color tileIndexColor = GetImageColor(wangTilesetImg, x, y);
                uint8_t tile = tileIndexColor.r % map.tileDefCount;
                Color color = map.TileDefAvgColor(tile);
                ImageDrawPixel(&colorized, x, y, color);
            }
        }
    }
    thumbnail = LoadTextureFromImage(colorized);
    UnloadImage(colorized);
}
#endif

Err WangTileset::Load(Tilemap &map, const char *filename)
{
    Image wangTilesetImg = LoadImage(filename);
    if (wangTilesetImg.width) {
        ImageFormat(&wangTilesetImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        if (!stbhw_build_tileset_from_image(&ts, wangTilesetImg)) {
            printf("[wang_tileset] Failed to build tileset from image %s. %s\n", filename, stbhw_get_last_error());
            return RN_BAD_FILE_READ;
        }

        assert(ts.num_h_tiles);
        assert(ts.num_v_tiles);

        if (!ts.num_h_tiles || !ts.num_v_tiles) {
            printf("[wang_tileset] Tileset must have at least 1 horizontal and vertical tile each. hor: %d ver: %d\n", ts.num_h_tiles, ts.num_v_tiles);
            return RN_BAD_FILE_READ;
        }

        this->filename = filename;
        thumbnail = GenerateColorizedTexture(ts.img, map);

        //hWangTilemaps.resize(ts.num_h_tiles);
        //vWangTilemaps.resize(ts.num_v_tiles);
    }
    return RN_SUCCESS;
}

void WangTileset::Unload(void)
{
    UnloadTexture(thumbnail);
    UnloadImage(ts.img);
    stbhw_free_tileset(&ts);
}

Err WangTileset::GenerateMap(uint32_t w, uint32_t h, Tilemap &map, WangMap &wangMap)
{
    Err err = RN_SUCCESS;

    if (!ts.num_h_tiles || !ts.num_v_tiles) {
        printf("[wang_tileset] Cannot generate map without tiles. Did you load the tileset first?\n");
        return RN_INVALID_SIZE;
    }

    UnloadImage(wangMap.image);
    UnloadTexture(wangMap.indexed);
    UnloadTexture(wangMap.colorized);

    unsigned char *pixels = (unsigned char *)calloc((size_t)3 * w * h, 1);
    do {
        if (!pixels) {
            err = RN_BAD_ALLOC; break;
        }

        if (!stbhw_generate_image(&ts, 0, pixels, w*3, w, h)) {
            free(pixels);
            printf("[wang_tileset] Failed to generate map from tileset. %s\n", stbhw_get_last_error());
            err = RN_BAD_ALLOC; break;
        }

        Image img{};
        img.width = w;
        img.height = h;
        img.format = format;
        img.data = pixels;
        img.mipmaps = 1;
        ImageColorGrayscale(&img);

        wangMap.image = img;
        wangMap.indexed = LoadTextureFromImage(wangMap.image);
        wangMap.colorized = GenerateColorizedTexture(wangMap.image, map);
    } while (0);

    return err;
}