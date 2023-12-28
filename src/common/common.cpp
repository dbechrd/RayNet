#include "common.h"
#include "data.h"

#define RAYMATH_IMPLEMENTATION
#include "raylib/raymath.h"
#include "raylib/rlgl.h"

#define STB_HERRINGBONE_WANG_TILE_IMPLEMENTATION
#include "stb_herringbone_wang_tile.cpp"

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
    int fileSize = 0;
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

    if (text == NULL || textLen == 0 || font.texture.id == 0) {
        return textSize;
    }

    const float spacing = 1.0f;

    Vector2 cur{};
    if (!cursor) {
        cursor = &cur;
    }

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
            if (i < textLen - 1 && text[i] != '\n') {
                cursor->x += spacing;
            }
        }

        if (codepoint != '\r') {
            prevCodepoint = codepoint;
        }
    }

    textWidth = MAX(cursor->x, textWidth);
    textHeight += font.baseSize;

    textSize.x = textWidth; // Adds chars spacing to measure
    textSize.y = textHeight;

    return textSize;
}

void dlb_DrawTextEx(Font font, const char *text, size_t textLen, Vector2 position, Color tint, Vector2 *cursor, bool *hovered)
{
    if (font.texture.id == 0) {
        font = GetFontDefault();
    }

    const float spacing = 1.0f;

    Vector2 cur{};
    if (!cursor) {
        cursor = &cur;
    }

    // TODO(dlb): No idea why Raylib casts this to int.. *shrugs*
    const float charHeight = (int)(font.baseSize * TEXT_LINE_SPACING);

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

            const float advanceXWithSpacing = advanceX + spacing;
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
        DrawTextCodepoint(font, cmd.codepoint, cmd.pos, font.baseSize, col);
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

void dlb_DrawTextShadowEx(Font font, const char *text, size_t textLen, Vector2 pos, Color color)
{
    //BeginShaderMode(shdSdfText);
    Vector2 shadowPos = Vector2Add(pos, { 1, 1 });
    if (font.baseSize > 32) {
        shadowPos = Vector2AddValue(pos, 2);
    }

    if (color.r || color.g || color.b) {
        // Don't draw shadows on black text
        dlb_DrawTextEx(font, text, textLen, shadowPos, { 0, 0, 0, color.a });
    }
    dlb_DrawTextEx(font, text, textLen, pos, color);
    //EndShaderMode();
}

Rectangle GetCameraRectWorld(Camera2D &camera)
{
#if CL_DBG_TILE_CULLING
    const int screenMargin = 64;
    Vector2 screenTLWorld = GetScreenToWorld2D({ screenMargin, screenMargin }, camera);
    Vector2 screenBRWorld = GetScreenToWorld2D({ (float)GetRenderWidth() - screenMargin, (float)GetRenderHeight() - screenMargin }, camera);
#else
    const Vector2 cameraTLWorld = GetScreenToWorld2D({ 0, 0 }, camera);
    const Vector2 cameraBRWorld = GetScreenToWorld2D({ (float)GetRenderWidth(), (float)GetRenderHeight() }, camera);
#endif

    const Rectangle cameraRectWorld{
        cameraTLWorld.x,
        cameraTLWorld.y,
        cameraBRWorld.x - cameraTLWorld.x,
        cameraBRWorld.y - cameraTLWorld.y,
    };
    return cameraRectWorld;
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

void RectConstrainToScreen(Rectangle &rect, Vector2 *resultOffset)
{
    Vector2 screenSize{ (float)GetRenderWidth(), (float)GetRenderHeight() };
    Rectangle newRect = rect;
    newRect.x = CLAMP(rect.x, 0, screenSize.x - rect.width);
    newRect.y = CLAMP(rect.y, 0, screenSize.y - rect.height);
    if (resultOffset) {
        *resultOffset = { newRect.x - rect.x, newRect.y - rect.y };
    }
    rect = newRect;
}

void CircleConstrainToScreen(Vector2 &center, float radius, Vector2 *resultOffset)
{
    Vector2 screenSize{ (float)GetRenderWidth(), (float)GetRenderHeight() };
    Vector2 newCenter = center;
    newCenter.x = CLAMP(newCenter.x, radius, screenSize.x - radius);
    newCenter.y = CLAMP(newCenter.y, radius, screenSize.y - radius);
    if (resultOffset) {
        *resultOffset = { newCenter.x - center.x, newCenter.y - center.y };
    }
    center = newCenter;
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
        rlTexCoord2f(source.x/width + 0.0001f, source.y/height + 0.0001f);
        rlVertex2f(topLeft.x, topLeft.y);

        // Bottom-left corner for texture and quad
        rlTexCoord2f(source.x/width + 0.0001f, (source.y + source.height)/height - 0.0001f);
        rlVertex2f(bottomLeft.x, bottomLeft.y);

        // Bottom-right corner for texture and quad
        rlTexCoord2f((source.x + source.width)/width - 0.0001f, (source.y + source.height)/height - 0.0001f);
        rlVertex2f(bottomRight.x, bottomRight.y);

        // Top-right corner for texture and quad
        rlTexCoord2f((source.x + source.width)/width - 0.0001f, source.y/height + 0.0001f);
        rlVertex2f(topRight.x, topRight.y);

        rlEnd();
        rlSetTexture(0);
    }
}

void dlb_DrawNPatch(Rectangle rec)
{
    const GfxFile &gfx_file = packs[0].FindByName<GfxFile>("gfx_dlg_npatch");
    NPatchInfo nPatch{};
    nPatch.source = { 0, 0, (float)gfx_file.texture.width, (float)gfx_file.texture.height };
    nPatch.left   = 16;
    nPatch.top    = 16;
    nPatch.right  = 16;
    nPatch.bottom = 16;
    nPatch.layout = NPATCH_NINE_PATCH;
    DrawTextureNPatch(gfx_file.texture, nPatch, rec, {}, 0, WHITE);
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
#include "entity.cpp"
#include "entity_db.cpp"
#include "error.cpp"
#include "file_utils.cpp"
#include "haq.cpp"
#include "histogram.cpp"
#include "lz4.c"
#include "io.cpp"
#include "net/net.cpp"
#include "perf_timer.cpp"
#include "strings.cpp"
#include "tilemap.cpp"
#include "ui/ui.cpp"
#include "uid.cpp"
#include "wang.cpp"