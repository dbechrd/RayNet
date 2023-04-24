#include "tilemap.h"
#include "collision.h"

static TileDef hardcodedTiles32TileDefs[] = {
#define TILEDEF(y, x, collide) { x * (TILE_W + 2) + 1, y * (TILE_W + 2) + 1, collide }
    TILEDEF(0, 0, 0),  // void
    TILEDEF(0, 1, 0),  // bright grass
    TILEDEF(0, 2, 0),  // bright water
    TILEDEF(0, 3, 0),
    TILEDEF(0, 4, 0),
    TILEDEF(1, 0, 0),
    TILEDEF(1, 1, 0),
    TILEDEF(1, 2, 0),
    TILEDEF(1, 3, 0),
    TILEDEF(1, 4, 0),
    TILEDEF(2, 0, 0),
    TILEDEF(2, 1, 0),
    TILEDEF(2, 2, 0),
    TILEDEF(2, 3, 0),
    TILEDEF(2, 4, 0),
    TILEDEF(3, 0, 0),
    TILEDEF(3, 1, 0),
    TILEDEF(3, 2, 0),
    TILEDEF(3, 3, 0),
    TILEDEF(3, 4, 1),  // dark water
#undef TILEDEF
};

Err Tilemap::Alloc(int version, int width, int height, const char *texturePath)
{
    version = 1;
    if (width <= 0 || height <= 0 || width > 128 || height > 128) {
        return RN_INVALID_SIZE;
    }

    if (!texturePath) {
        return RN_INVALID_PATH;
    }
    texture = LoadTexture(texturePath);
    if (!texture.width) {
        return RN_BAD_FILE_READ;
    }

    // TODO: Load this from somewhere... a tilemap.dat file or something.
    tileDefCount = ARRAY_SIZE(hardcodedTiles32TileDefs);
    tileDefs = hardcodedTiles32TileDefs;

    tiles = (Tile *)calloc((size_t)width * height, sizeof(*tiles));
    if (!tiles) {
        return RN_BAD_ALLOC;
    }

    return RN_SUCCESS;
}

void Tilemap::Free(void)
{
    UnloadTexture(texture);
    free(texturePath);
    free(tiles);
}

Tilemap::~Tilemap(void)
{
    Free();
}

Err Tilemap::Save(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file) {
        assert(width);
        assert(height);
        assert(texturePath);
        assert(tiles);

        fwrite(&MAGIC, sizeof(MAGIC), 1, file);
        fwrite(&VERSION, sizeof(VERSION), 1, file);
        uint32_t texturePathLen = (uint32_t)strlen(texturePath);
        fwrite(&texturePathLen, sizeof(texturePathLen), 1, file);
        fwrite(texturePath, sizeof(*texturePath), texturePathLen, file);
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
    Free();

    FILE *file = fopen(filename, "r");
    if (!file) {
        return RN_BAD_FILE_READ;
    }

    uint32_t magic = 0;
    fread(&magic, sizeof(magic), 1, file);
    if (magic != MAGIC) {
        return RN_BAD_MAGIC;
    }

    uint32_t version = 0;
    fread(&version, sizeof(version), 1, file);

    if (version >= 2) {
        uint32_t texturePathLen = 0;
        fread(&texturePathLen, sizeof(texturePathLen), 1, file);
        if (!texturePathLen || texturePathLen > 1024) {
            return RN_INVALID_PATH;
        }
        texturePath = (char *)calloc(texturePathLen, sizeof(*texturePath));
        if (!texturePath) {
            return RN_BAD_ALLOC;
        }
        fread(texturePath, sizeof(*texturePath), texturePathLen, file);
    } else {
        const char *v1_texturePath = "resources/tiles32.png";
        const size_t v1_texturePathLen = strlen(v1_texturePath);
        texturePath = (char *)calloc(v1_texturePathLen + 1, sizeof(*v1_texturePath));
        if (!texturePath) {
            return RN_BAD_ALLOC;
        }
        strncpy(texturePath, v1_texturePath, v1_texturePathLen);
    }

    texture = LoadTexture(texturePath);
    if (!texture.width) {
        return RN_BAD_FILE_READ;
    }

    // TODO: Load this from somewhere... a tilemap.dat file or something.
    tileDefCount = ARRAY_SIZE(hardcodedTiles32TileDefs);
    tileDefs = hardcodedTiles32TileDefs;

    int oldWidth = width;
    int oldHeight = height;
    fread(&width, sizeof(width), 1, file);
    fread(&height, sizeof(height), 1, file);
    if (!width || !height) {
        return RN_INVALID_SIZE;
    }

    const size_t tileCount = (size_t)width * height;
    tiles = (Tile *)calloc(tileCount, sizeof(*tiles));
    if (!tiles) {
        return RN_BAD_ALLOC;
    }
    fread(tiles, sizeof(*tiles), tileCount, file);

    fclose(file);
    return RN_SUCCESS;
}

Tile Tilemap::At(int x, int y)
{
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
    return tiles[y * width + x];
}

bool Tilemap::AtTry(int x, int y, Tile &tile)
{
    if (x >= 0 && y >= 0 && x < width && y < height) {
        tile = At(x, y);
        return true;
    }
    return false;
}

bool Tilemap::WorldToTileIndex(int world_x, int world_y, Coord &coord)
{
    if (world_x >= 0 && world_y >= 0 && world_x < width * TILE_W && world_y < height * TILE_W) {
        coord.x = world_x / TILE_W;
        coord.y = world_y / TILE_W;
        return true;
    }
    return false;
}

bool Tilemap::AtWorld(int world_x, int world_y, Tile &tile)
{
    Coord coord{};
    if (WorldToTileIndex(world_x, world_y, coord)) {
        tile = At(coord.x, coord.y);
        return true;
    }
    return false;
}

void Tilemap::Set(int x, int y, Tile tile)
{
    assert(x >= 0);
    assert(y >= 0);
    assert(x < width);
    assert(y < height);
    tiles[y * width + x] = tile;
}

void Tilemap::ResolveEntityTerrainCollisions(Entity &entity)
{
    if (!entity.radius) {
        return;
    }

    entity.colliding = false;

    Vector2 topLeft{
        entity.position.x - entity.radius,
        entity.position.y - entity.radius
    };
    Vector2 bottomRight{
        entity.position.x + entity.radius,
        entity.position.y + entity.radius
    };

    int yMin = CLAMP(floorf(topLeft.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf(bottomRight.y / TILE_W), 0, height);
    int xMin = CLAMP(floorf(topLeft.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf(bottomRight.x / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile{};
            if (AtTry(x, y, tile)) {
                TileDef &tileDef = tileDefs[tile];
                if (tileDef.collide) {
                    Rectangle tileRect{};
                    Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
                    tileRect.x = tilePos.x;
                    tileRect.y = tilePos.y;
                    tileRect.width = TILE_W;
                    tileRect.height = TILE_W;
                    Vector2 contact{};
                    Vector2 normal{};
                    float depth{};
                    if (dlb_CheckCollisionCircleRec(entity.position, entity.radius, tileRect, contact, normal, depth)) {
                        entity.colliding = true;
                        if (Vector2DotProduct(entity.velocity, normal) < 0) {
                            entity.position.x += normal.x * depth;
                            entity.position.y += normal.y * depth;
                        }
                    }
                }
            }
        }
    }
}

void Tilemap::Draw(Camera2D &camera)
{
    // [World] Tilemap
#if CL_DBG_TILE_CULLING
    const int screenMargin = 64;
    Vector2 screenTLWorld = GetScreenToWorld2D({ screenMargin, screenMargin }, camera);
    Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth() - screenMargin, (float)GetScreenHeight() - screenMargin }, camera);
#else
    Vector2 screenTLWorld = GetScreenToWorld2D({ 0, 0 }, camera);
    Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera);
#endif

    int yMin = CLAMP(floorf(screenTLWorld.y / TILE_W), 0, height);
    int yMax = CLAMP(ceilf(screenBRWorld.y / TILE_W), 0, height);
    int xMin = CLAMP(floorf(screenTLWorld.x / TILE_W), 0, width);
    int xMax = CLAMP(ceilf(screenBRWorld.x / TILE_W), 0, width);

    for (int y = yMin; y < yMax; y++) {
        for (int x = xMin; x < xMax; x++) {
            Tile tile = At(x, y);
            TileDef &tileDef = tileDefs[tile];

            Rectangle texRect{ (float)tileDef.x, (float)tileDef.y, TILE_W, TILE_W };
            Vector2 tilePos = { (float)x * TILE_W, (float)y * TILE_W };
            DrawTextureRec(texture, texRect, tilePos, WHITE);
        }
    }
}