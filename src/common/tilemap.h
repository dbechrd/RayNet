#pragma once
#include "common.h"
#include <cstdio>

typedef uint8_t Tile;

struct Tilemap {
    struct Coord {
        int x, y;
    };

    const uint32_t MAGIC = 0xDBBB9192;
    int version;
    int width;
    int height;
    Tile *tiles;

    Err Alloc(int version, int width, int height);
    ~Tilemap();

    Err Save(const char *filename);
    Err Load(const char *filename);
    Tile At(int x, int y);
    bool AtTry(int x, int y, Tile &tile);
    bool WorldToTileIndex(int world_x, int world_y, Coord &coord);
    bool AtWorld(int world_x, int world_y, Tile &tile);
    void Set(int x, int y, Tile tile);
};