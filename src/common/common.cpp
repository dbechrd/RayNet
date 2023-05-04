#include "common.h"
#include "audio/audio.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"

Shader shdSdfText;
Font fntHackBold20;
Font fntHackBold32;

Texture texLily;

Sound sndSoftTick;
Sound sndHardTick;

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

    sndSoftTick = LoadSound("resources/soft_tick.wav");
    if (!sndSoftTick.frameCount) err = RN_RAYLIB_ERROR;

    sndHardTick = LoadSound("resources/hard_tick.wav");
    if (!sndHardTick.frameCount) err = RN_RAYLIB_ERROR;

    rnSoundSystem.Init();

    return err;
}

void FreeCommon(void)
{
    UnloadShader(shdSdfText);
    UnloadFont(fntHackBold20);
    UnloadFont(fntHackBold32);
    UnloadTexture(texLily);
    UnloadSound(sndSoftTick);
    UnloadSound(sndHardTick);
    rnSoundSystem.Free();
}