#include "common.h"
#include "audio/audio.h"
#include "texture_catalog.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"

//#define STB_HERRINGBONE_WANG_TILE_IMPLEMENTATION
//#include "stb_herringbone_wang_tile.c"

Shader shdSdfText;
Font fntHackBold20;
Font fntHackBold32;

Texture texLily;

Music musAmbientOutdoors;
Music musCave;

IO io;

const char *ErrStr(Err err)
{
    switch (err) {
        case RN_SUCCESS         : return "RN_SUCCESS        ";
        case RN_BAD_ALLOC       : return "RN_BAD_ALLOC      ";
        case RN_BAD_MAGIC       : return "RN_BAD_MAGIC      ";
        case RN_BAD_FILE_READ   : return "RN_BAD_FILE_READ  ";
        case RN_BAD_FILE_WRITE  : return "RN_BAD_FILE_WRITE ";
        case RN_INVALID_SIZE    : return "RN_INVALID_SIZE   ";
        case RN_INVALID_PATH    : return "RN_INVALID_PATH   ";
        case RN_NET_INIT_FAILED : return "RN_NET_INIT_FAILED";
        case RN_INVALID_ADDRESS : return "RN_INVALID_ADDRESS";
        case RN_RAYLIB_ERROR    : return "RN_RAYLIB_ERROR   ";
        case RN_BAD_ID          : return "RN_BAD_ID         ";
        case RN_OUT_OF_BOUNDS   : return "RN_OUT_OF_BOUNDS  ";
        default                 : return TextFormat("Code %d", err);
    }
}

// TODO: Load placeholder textures/sounds etc. if fail
Err InitCommon(void)
{
    Err err = RN_SUCCESS;

    // Load SDF required shader (we use default vertex shader)
    shdSdfText = LoadShader(0, "resources/shaders/sdf.fs");

    fntHackBold20 = LoadFontEx("resources/Hack-Bold.ttf", 20, 0, 0);
    if (!fntHackBold20.baseSize) err = RN_RAYLIB_ERROR;

    fntHackBold32 = dlb_LoadFontEx("resources/Hack-Bold.ttf", 42, 0, 0, FONT_SDF);
    if (!fntHackBold32.baseSize) err = RN_RAYLIB_ERROR;
    SetTextureFilter(fntHackBold32.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    texLily = LoadTexture("resources/lily.png");
    if (!texLily.width) err = RN_RAYLIB_ERROR;

    musAmbientOutdoors = LoadMusicStream("resources/copyright/345470__philip_goddard__branscombe-landslip-birds-and-sea-echoes-ese-from-cave-track.ogg");
    musCave = LoadMusicStream("resources/copyright/69391__zixem__cave_amb.wav");

    rnSoundSystem.Init();
    rnTextureCatalog.Init();
    rnSpritesheetCatalog.Init();
    rnSpritesheetCatalog.FindOrLoad("resources/campfire.txt");

    return err;
}

void FreeCommon(void)
{
    UnloadShader(shdSdfText);
    UnloadFont(fntHackBold20);
    UnloadFont(fntHackBold32);
    UnloadTexture(texLily);
    UnloadMusicStream(musAmbientOutdoors);
    UnloadMusicStream(musCave);
    rnSpritesheetCatalog.Free();
    rnTextureCatalog.Free();
    rnSoundSystem.Free();
}

// Load font from memory buffer, fileType refers to extension: i.e. ".ttf"
Font dlb_LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int glyphCount, int type)
{
    Font font = { 0 };

    char fileExtLower[16] = { 0 };
    strcpy(fileExtLower, TextToLower(fileType));

    if (TextIsEqual(fileExtLower, ".ttf") ||
        TextIsEqual(fileExtLower, ".otf"))
    {
        font.baseSize = fontSize;
        font.glyphCount = (glyphCount > 0)? glyphCount : 95;
        font.glyphPadding = 0;
        font.glyphs = LoadFontData(fileData, dataSize, font.baseSize, fontChars, font.glyphCount, type);

        if (font.glyphs != NULL)
        {
            font.glyphPadding = 4;

            Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount, font.baseSize, font.glyphPadding, 0);
            font.texture = LoadTextureFromImage(atlas);

            // Update glyphs[i].image to use alpha, required to be used on ImageDrawText()
            for (int i = 0; i < font.glyphCount; i++)
            {
                UnloadImage(font.glyphs[i].image);
                font.glyphs[i].image = ImageFromImage(atlas, font.recs[i]);
            }

            UnloadImage(atlas);

            TraceLog(LOG_INFO, "FONT: Data loaded successfully (%i pixel size | %i glyphs)", font.baseSize, font.glyphCount);
        }
        else font = GetFontDefault();
    }

    return font;
}

// Load Font from TTF font file with generation parameters
// NOTE: You can pass an array with desired characters, those characters should be available in the font
// if array is NULL, default char set is selected 32..126
Font dlb_LoadFontEx(const char *fileName, int fontSize, int *fontChars, int glyphCount, int type)
{
    Font font = { 0 };

    // Loading file to memory
    unsigned int fileSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &fileSize);

    if (fileData != NULL)
    {
        // Loading font from memory data
        font = dlb_LoadFontFromMemory(GetFileExtension(fileName), fileData, fileSize, fontSize, fontChars, glyphCount, type);

        UnloadFileData(fileData);
    }
    else font = GetFontDefault();

    return font;
}

float GetRandomFloatZeroToOne(void)
{
    return (float)rand() / RAND_MAX;
}

float GetRandomFloatMinusOneToOne(void)
{
    return GetRandomFloatZeroToOne() * 2.0f - 1.0f;
}

float GetRandomFloatVariance(float variance)
{
    return GetRandomFloatMinusOneToOne() * variance;
}

void RandTests(void)
{
    float aMin = FLT_MAX;
    float aMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatZeroToOne();
        aMin = MIN(aMin, f);
        aMax = MAX(aMax, f);
    }
    printf("GetRandomFloatZeroToOne: %.2f - %.2f\n", aMin, aMax);

    float bMin = FLT_MAX;
    float bMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatMinusOneToOne();
        bMin = MIN(bMin, f);
        bMax = MAX(bMax, f);
    }
    printf("GetRandomFloatMinusOneToOne: %.2f - %.2f\n", bMin, bMax);

    float cMin = FLT_MAX;
    float cMax = FLT_MIN;
    for (int i = 0; i < 10000; i++) {
        float f = GetRandomFloatVariance(0.2f);
        cMin = MIN(cMin, f);
        cMax = MAX(cMax, f);
    }
    printf("GetRandomFloatVariance(0.2f): %.2f - %.2f\n", cMin, cMax);
}

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color)
{
#if 1
    Vector2 shadowPos = Vector2Add(pos, { 1, 1 });
    if (font.baseSize > 32) {
        shadowPos = Vector2AddValue(pos, 2);
    }

    //DrawRectangle(windowWidth / 2 - textSize.x / 2, 370, textSize.x, textSize.y, WHITE);
    if (color.r || color.g || color.b) {
        // Don't draw shadows on black text
        DrawTextEx(font, text, shadowPos, font.baseSize, 1, BLACK);
    }
    DrawTextEx(font, text, pos, font.baseSize, 1, color);
#else
    DrawTextEx(font, text, pos, font.baseSize, 1, color);
#endif
}

Rectangle GetScreenRectWorld(Camera2D &camera)
{
#if CL_DBG_TILE_CULLING
    const int screenMargin = 64;
    Vector2 screenTLWorld = GetScreenToWorld2D({ screenMargin, screenMargin }, camera);
    Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetRenderWidth() - screenMargin, (float)GetRenderHeight() - screenMargin }, camera);
#else
    const Vector2 screenTLWorld = GetScreenToWorld2D({ 0, 0 }, camera);
    const Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetRenderWidth(), (float)GetRenderHeight() }, camera);
#endif

    const Rectangle rectWorld{
        screenTLWorld.x,
        screenTLWorld.y,
        screenBRWorld.x - screenTLWorld.x,
        screenBRWorld.y - screenTLWorld.y,
    };
    return rectWorld;
}

#ifdef TRACY_ENABLE
#include "tracy/tracy/Tracy.hpp"

void *operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
void operator delete(void *ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}
#endif

Rectangle RectShrink(const Rectangle &rect, float pixels)
{
    assert(pixels * 2 < rect.width);
    assert(pixels * 2 < rect.height);
    Rectangle shrunk = rect;
    shrunk.x += pixels;
    shrunk.y += pixels;
    shrunk.width -= pixels * 2;
    shrunk.height -= pixels * 2;
    return shrunk;
}

Rectangle RectGrow(const Rectangle &rect, float pixels)
{
    Rectangle grown = rect;
    grown.x -= pixels;
    grown.y -= pixels;
    grown.width += pixels * 2;
    grown.height += pixels * 2;
    return grown;
}

#include "audio/audio.cpp"
#include "net/net.cpp"
#include "ui/ui.cpp"
#include "collision.cpp"
#include "editor.cpp"
#include "entity.cpp"
#include "file_utils.cpp"
#include "histogram.cpp"
#include "spritesheet.cpp"
#include "texture_catalog.cpp"
#include "tilemap.cpp"
#include "wang.cpp"
