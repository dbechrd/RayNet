#include "common.h"
#include "data.h"
#include "texture_catalog.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"
#include "raylib/rlgl.h"

//#define STB_HERRINGBONE_WANG_TILE_IMPLEMENTATION
//#include "stb_herringbone_wang_tile.c"

Shader shdSdfText;
Shader shdPixelFixer;
int    shdPixelFixerScreenSizeUniformLoc;

Font fntTiny;
Font fntSmall;
Font fntMedium;
Font fntBig;

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

// TODO: LoadPack placeholder textures/sounds etc. if fail
Err InitCommon(void)
{
    Err err = RN_SUCCESS;
    data::Init();
    rnStringCatalog.Init();
    rnTextureCatalog.Init();

    // LoadPack SDF required shader (we use default vertex shader)
    shdSdfText = LoadShader(0, "resources/shader/sdf.fs");

    shdPixelFixer                     = LoadShader("resources/shader/pixelfixer.vs", "resources/shader/pixelfixer.fs");
    shdPixelFixerScreenSizeUniformLoc = GetShaderLocation(shdPixelFixer, "screenSize");

#if 0
    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    if (!FileExists(fontName)) {
        fontName = "resources/font/KarminaBold.otf";
    }
#else
    //const char *fontName = "resources/font/KarminaBold.otf";
    const char *fontName = "resources/font/PixelOperator-Bold.ttf";
    //const char *fontName = "resources/font/PixelOperatorMono-Bold.ttf";
#endif

    fntTiny = dlb_LoadFontEx(fontName, 14, 0, 0, FONT_DEFAULT);
    if (!fntTiny.baseSize) err = RN_RAYLIB_ERROR;

    fntSmall = dlb_LoadFontEx(fontName, 18, 0, 0, FONT_DEFAULT);
    if (!fntSmall.baseSize) err = RN_RAYLIB_ERROR;

    fntMedium = dlb_LoadFontEx(fontName, 20, 0, 0, FONT_DEFAULT);
    if (!fntMedium.baseSize) err = RN_RAYLIB_ERROR;

    fntBig = dlb_LoadFontEx(fontName, 46, 0, 0, FONT_DEFAULT);
    if (!fntBig.baseSize) err = RN_RAYLIB_ERROR;
    //SetTextureFilter(fntBig.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    return err;
}

void FreeCommon(void)
{
    UnloadShader(shdSdfText);
    UnloadFont(fntSmall);
    UnloadFont(fntMedium);
    UnloadFont(fntBig);
    rnTextureCatalog.Free();
    data::Free();
}

// LoadPack font from memory buffer, fileType refers to extension: i.e. ".ttf"
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

// LoadPack Font from TTF font file with generation parameters
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

Vector2 dlb_MeasureTextEx(Font font, const char *text, size_t textLen, Vector2 *cursor)
{
    Vector2 textSize{};

    if ((font.texture.id == 0) || (text == NULL)) {
        return textSize;
    }

    Vector2 cur{};
    if (!cursor) {
        cursor = &cur;
    }

    int lineCharsMax = 0;
    int lineChars = 0;

    float textWidth = 0.0f;
    float textHeight = 0.0f;
    const float charHeight = font.baseSize * TEXT_LINE_SPACING;  // including line spacing, for bbox

    int prevCodepoint = 0;
    for (int i = 0; i < textLen; i++) {
        int codepointSize = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointSize);
        int glyphIndex = GetGlyphIndex(font, codepoint);

        // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all the bad bytes using the '?' symbol so to not skip any we set codepointSize = 1
        if (codepoint == 0x3f) {
            codepointSize = 1;
        }
        i += codepointSize - 1;

        if (codepoint == '\\') {
            // don't measure
        } else if (codepoint == '\n') {
            textWidth = MAX(cursor->x, textWidth);
            lineChars = 0;
            cursor->x = 0;

            float lineHeight = charHeight;
            if (prevCodepoint == '\n') {
                lineHeight /= 2;  // half space on double newlines
            }
            cursor->y += lineHeight;
            textHeight += lineHeight;
        } else {
            float advanceX = font.glyphs[glyphIndex].advanceX;
            if (!advanceX) {
                advanceX = font.recs[glyphIndex].width + font.glyphs[glyphIndex].offsetX;
            }
            cursor->x += advanceX;
            lineChars++;
        }

        lineCharsMax = MAX(lineChars, lineCharsMax);
        if (codepoint != '\r') {
            prevCodepoint = codepoint;
        }
    }

    textWidth = MAX(cursor->x, textWidth);
    textHeight += font.baseSize;

    const float fontSize = font.baseSize;
    const float spacing = 1.0f;

    float scaleFactor = fontSize / font.baseSize;
    textSize.x = textWidth * scaleFactor + ((lineCharsMax - 1) * spacing); // Adds chars spacing to measure
    textSize.y = textHeight * scaleFactor;

    return textSize;
}

void dlb_DrawTextEx(Font font, const char *text, size_t textLen, Vector2 position, Color tint, Vector2 *cursor, bool *hovered)
{
    if (font.texture.id == 0) {
        font = GetFontDefault();
    }

    const float fontSize = font.baseSize;
    const float spacing = 1.0f;

    Vector2 cur{};
    if (!cursor) {
        cursor = &cur;
    }

    float scaleFactor = fontSize / font.baseSize; // Character quad scaling factor
    // TODO(dlb): No idea why Raylib casts this to int.. *shrugs*
    const float charHeight = (int)((font.baseSize * TEXT_LINE_SPACING) * scaleFactor);

    struct GlyphDrawCmd {
        int codepoint;
        Vector2 pos;

        GlyphDrawCmd(int codepoint, Vector2 pos) : codepoint(codepoint), pos(pos) {}
    };
    std::vector<GlyphDrawCmd> glyphDrawCmds{};

    int prevCodepoint = 0;
    for (int i = 0; i < textLen;)
    {
        // Get codepointSize codepoint from byte string and glyph glyphIndex in font
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) {
            codepointByteCount = 1;
        }

        if (codepoint == '\\') {
            // don't draw
        } else if (codepoint == '\n') {
            float lineHeight = charHeight;
            if (prevCodepoint == '\n') {
                lineHeight /= 2;  // half space on double newlines
            }
            cursor->y += lineHeight;
            cursor->x = 0.0f;
        } else {
            const Vector2 glyphPos{ position.x + cursor->x, position.y + cursor->y };

            float advanceX = font.glyphs[index].advanceX;
            if (!advanceX) {
                advanceX = font.recs[index].width;
            }

            const float advanceXWithSpacing = advanceX * scaleFactor + spacing;
            cursor->x += advanceXWithSpacing;

            if (hovered) {
                const Rectangle glyphRec{
                    glyphPos.x, glyphPos.y,
                    advanceXWithSpacing, charHeight
                };

                if (dlb_CheckCollisionPointRec(GetMousePosition(), glyphRec)) {
                    *hovered = true;
                }
            }

            if ((codepoint != ' ') && (codepoint != '\t')) {
                glyphDrawCmds.emplace_back(codepoint, glyphPos);
            }
        }

        i += codepointByteCount;   // Move text bytes counter to codepointSize codepoint
        if (codepoint != '\r') {
            prevCodepoint = codepoint;
        }
    }

    Color col = (hovered && *hovered) ? ColorBrightness(tint, 0.5f) : tint;
    for (GlyphDrawCmd &cmd : glyphDrawCmds) {
        DrawTextCodepoint(font, cmd.codepoint, cmd.pos, fontSize, col);
    }
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
    //BeginShaderMode(shdSdfText);
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
    //EndShaderMode();
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

Rectangle RectConstrainToScreen(const Rectangle &rect, Vector2 *resultOffset)
{
    Vector2 screenSize{ (float)GetRenderWidth(), (float)GetRenderHeight() };
    Rectangle newRect = rect;
    newRect.x = CLAMP(rect.x, 0, screenSize.x - rect.width);
    newRect.y = CLAMP(rect.y, 0, screenSize.y - rect.height);
    if (resultOffset) {
        *resultOffset = { newRect.x - rect.x, newRect.y - rect.y };
    }
    return newRect;
}

// Draw a part of a texture (defined by a rectangle)
void dlb_DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint)
{
    Rectangle dest = { position.x, position.y, source.width, source.height };
    Vector2 origin = { 0.0f, 0.0f };

    dlb_DrawTexturePro(texture, source, dest, origin, tint);
}

// Draw a part of a texture (defined by a rectangle) with 'pro' parameters
// NOTE: origin is relative to destination rectangle size
void dlb_DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, Color tint)
{
    // Check if texture is valid
    if (texture.id > 0)
    {
        float x = dest.x - origin.x;
        float y = dest.y - origin.y;
        float width = (float)texture.width;
        float height = (float)texture.height;

        Vector2 topLeft = { x, y };
        Vector2 topRight = { x + dest.width, y };
        Vector2 bottomLeft = { x, y + dest.height };
        Vector2 bottomRight = { x + dest.width, y + dest.height };

        rlSetTexture(texture.id);
        rlBegin(RL_QUADS);

        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        // Normal vector pointing towards viewer
        rlNormal3f(0.0f, 0.0f, 1.0f);

        // Top-left corner for texture and quad
        rlTexCoord2f(source.x/width, source.y/height);
        rlVertex2f(topLeft.x, topLeft.y);

        // Bottom-left corner for texture and quad
        rlTexCoord2f(source.x/width, (source.y + source.height)/height);
        rlVertex2f(bottomLeft.x, bottomLeft.y);

        // Bottom-right corner for texture and quad
        rlTexCoord2f((source.x + source.width)/width, (source.y + source.height)/height);
        rlVertex2f(bottomRight.x, bottomRight.y);

        // Top-right corner for texture and quad
        rlTexCoord2f((source.x + source.width)/width, source.y/height);
        rlVertex2f(topRight.x, topRight.y);

        rlEnd();
        rlSetTexture(0);
    }
}

void dlb_DrawNPatch(Rectangle rec)
{
    NPatchInfo nPatch{};
    Texture &nPatchTex = data::packs[0]->gfx_files[data::GFX_FILE_DLG_NPATCH].texture;
    nPatch.source = { 0, 0, (float)nPatchTex.width, (float)nPatchTex.height };
    nPatch.left = 16;
    nPatch.top = 16;
    nPatch.right = 16;
    nPatch.bottom = 16;
    nPatch.layout = NPATCH_NINE_PATCH;
    DrawTextureNPatch(nPatchTex, nPatch, rec, {}, 0, WHITE);
}

bool StrFilter(const char *str, const char *filter)
{
    if (!filter || !*filter) {
        return false;
    }
    if (!strcmp(filter, "*")) {
        return true;
    }
    if (!str) {
        return false;
    }

    const char *s = str;
    const char *f = filter;
    while (*s) {
        if (std::tolower(*s++) == std::tolower(*f++)) {
            if (!*f) {
                return true;
            }
        } else {
            f = filter;
        }
    }
    return false;
}

#if 0
bool dlb_FancyTextParse(FancyTextTree &tree, const char *text)
{
    if (!text) {
        return true;
    }

    int dialogOptionIdx = 0;

    FancyTextNode *node = 0;
    const char *c = text;
    while (*c) {
        if (!node) {
            node = &tree.nodes.emplace_back();
        }
        switch (*c) {
            case '{': {
                if (*(c + 1)) {
                    c++;  // skip '{'
                } else {
                    printf("Expected string after '{' at %zu\n", (c + 1) - text);
                    return false;
                }

                node->type = FancyTextNode::DIALOG_OPTION;
                node->text = c;
                node->optionId = dialogOptionIdx++;

                while (*c && *c != '}') {
                    node->textLen++;
                    c++;
                }

                if (*c == '}') {
                    c++;  // skip '}'
                } else {
                    printf("Expected '}' at %zu\n", c - text);
                    return false;
                }

                node = NULL;
                break;
            }
            case '[': {
                if (*(c + 1)) {
                    c++;  // skip '['
                } else {
                    printf("Expected string after '[' at %zu\n", (c + 1) - text);
                    return false;
                }

                node->type = FancyTextNode::HOVER_TIP;
                node->text = c;

                while (*c && *c != '|') {
                    node->textLen++;
                    c++;
                }

                if (*c == '|') {
                    c++;  // skip '|'
                } else {
                    printf("Expected '|' at %zu\n", c - text);
                    return false;
                }

                node->tip = c;

                while (*c && *c != ']') {
                    node->tipLen++;
                    c++;
                }

                if (*c == ']') {
                    c++;  // skip ']'
                } else {
                    printf("Expected ']' at %zu\n", c - text);
                    return false;
                }

                node = NULL;
                break;
            }
            default: {
                node->type = FancyTextNode::TEXT;
                node->text = c;

                while (*c && *c != '{' && *c != '[') {
                    node->textLen++;
                    c++;
                };

                node = NULL;
                break;
            }
        }
    }

#if 0
    for (FancyTextNode &node : tree.nodes) {
        const char *nodeType = "";
        switch (node.type) {
            case FancyTextNode::TEXT:          nodeType = "TEXT";          break;
            case FancyTextNode::DIALOG_OPTION: nodeType = "DIALOG_OPTION"; break;
            case FancyTextNode::HOVER_TIP:     nodeType = "HOVER_TIP";     break;
        }
        printf("%s:%.*s\n", nodeType, (int)node.textLen, node.text);
    }
    printf("--------\n");
#endif

    return true;
}
#endif

void dlb_CommonTests(void)
{
#if 0
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "hello friend!"));
        assert(tree.nodes.size() == 1);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "hello {name}"));
        assert(tree.nodes.size() == 2);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "hello {name}!"));
        assert(tree.nodes.size() == 3);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "{name} is a cool name!"));
        assert(tree.nodes.size() == 2);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "\"{name}\" is a cool name!"));
        assert(tree.nodes.size() == 3);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "{first_name}{last_name} is a cool name!"));
        assert(tree.nodes.size() == 3);
    }
    {
        FancyTextTree tree{};
        assert(dlb_FancyTextParse(tree, "Ow! You hit me!\n\n{Sorry}\n{Haha, loser}"));
        assert(tree.nodes.size() == 4);
    }
#endif
}

#include "collision.cpp"
#include "dlg.cpp"
#include "data.cpp"
#include "file_utils.cpp"
#include "entity_db.cpp"
#include "histogram.cpp"
#include "io.cpp"
#include "net/net.cpp"
#include "strings.cpp"
#include "texture_catalog.cpp"
#include "tilemap.cpp"
#include "ui/ui.cpp"
#include "uid.cpp"
#include "wang.cpp"
