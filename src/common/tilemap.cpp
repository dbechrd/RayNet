#include "tilemap.h"

Err Tilemap::Alloc(int version, int width, int height)
{
    version = 1;
    if (width <= 0 || height <= 0 || width > 128 || height > 128) {
        return RN_INVALID_SIZE;
    }

    tiles = (Tile *)calloc((size_t)width * height, sizeof(*tiles));
    if (!tiles) {
        return RN_BAD_ALLOC;
    }

    return RN_SUCCESS;
}

Tilemap::~Tilemap() {
    free(tiles);
}

Err Tilemap::Save(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file) {
        assert(width);
        assert(height);
        assert(tiles);

        fwrite(&MAGIC, sizeof(MAGIC), 1, file);
        fwrite(&version, sizeof(version), 1, file);
        fwrite(&width, sizeof(width), 1, file);
        fwrite(&height, sizeof(height), 1, file);
        fwrite(tiles, sizeof(*tiles), (size_t)width * height, file);
        fclose(file);
        return RN_SUCCESS;
    }
    return RN_BAD_FILE_WRITE;
}

Err Tilemap::Load(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file) {
        int magic = 0;
        fread(&magic, sizeof(magic), 1, file);
        if (magic != MAGIC) {
            return RN_BAD_MAGIC;
        }
        fread(&version, sizeof(version), 1, file);

        int oldWidth = width;
        int oldHeight = height;
        fread(&width, sizeof(width), 1, file);
        fread(&height, sizeof(height), 1, file);
        if (!width || !height) {
            return RN_INVALID_SIZE;
        } else if (width != oldWidth || height != oldHeight) {
            free(tiles);
            tiles = (Tile *)calloc((size_t)width * height, sizeof(*tiles));
        }

        if (!tiles) {
            return RN_BAD_ALLOC;
        }

        fread(tiles, sizeof(*tiles), (size_t)width * height, file);

        fclose(file);
        return RN_SUCCESS;
    }
    return RN_BAD_FILE_READ;
}

Tile Tilemap::At(int x, int y) {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
    return tiles[y * width + x];
}

bool Tilemap::AtTry(int x, int y, Tile &tile) {
    if (x >= 0 && y >= 0 && x < width && y < height) {
        tile = At(x, y);
        return true;
    }
    return false;
}

bool Tilemap::WorldToTileIndex(int world_x, int world_y, Coord &coord) {
    if (world_x >= 0 && world_y >= 0 && world_x < width * TILE_W && world_y < height * TILE_W) {
        coord.x = world_x / TILE_W;
        coord.y = world_y / TILE_W;
        return true;
    }
    return false;
}

bool Tilemap::AtWorld(int world_x, int world_y, Tile &tile) {
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile = At(coord.x, coord.y);
        return true;
    }
    return false;
}

void Tilemap::Set(int x, int y, Tile tile) {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
    tiles[y * width + x] = tile;
}
