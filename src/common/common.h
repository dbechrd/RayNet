#pragma once

#include "raylib/raylib.h"
#include "raylib/raymath.h"

#pragma warning(push, 0)
#include "yojimbo.h"
#pragma warning(pop)

#include "stb_herringbone_wang_tile.h"
#include "lz4.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <cctype>

#include <array>
#include <bitset>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "collision.h"
#include "draw_cmd.h"
#include "error.h"
#include "haq.h"
#include "ring_buffer.h"
#include "strings.h"
#include "ui/stb_textedit_config.h"
#include "wang.h"

// Stuff that probably shouldn't be here

//#define WINDOW_WIDTH 1920
//#define WINDOW_HEIGHT 1016

#define TEXT_LINE_SPACING 1.0f

#define TILE_W 64

#define TODO_LIST_PATH "todo.txt"

#define PATH_LEN_MAX 1024
#define MAP_OVERWORLD  "map_overworld"
#define MAP_CAVE       "map_cave"

// Developer macros

#define DEV_BUILD_PACK_FILE 1
#define DBG_UI_NO_SCISSOR   0
#define DBG_UI_SIGN_EDITOR  0

// Helper macros

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
//#define CLAMP(x, min, max) (MIN(MAX((x), (min)), (max)))
#define CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#define FIELD_SIZEOF(type, field) (sizeof(((type *)0)->field))
#define FIELD_SIZEOF_ARRAY(type, field) (sizeof(*(((type *)0)->field)))
#define OFFSETOF(type, field) ((size_t)&(((type *)0)->field))
#define STR(x) #x
#define CSTR(x) (x), (sizeof(x) - 1)
#define CSTRLEN(x) (x), strlen(x)
#define CSTRS(x) (x).data(), (x).size()
#define STRSTR(x) STR(x)

// Value
#define ENUM_V_VALUE(v) v,
// Value, Desc
#define ENUM_VD_VALUE(v, d) v,
// Value, Desc, Meta
#define ENUM_VDM_VALUE(v, d, m) v,

#define ENUM_V_CASE_RETURN_VSTR(v, ...) case v: return #v;
#define ENUM_VD_CASE_RETURN_DESC(v, d, ...) case v: return d;
#define ENUM_VDM_CASE_RETURN_META(v, d, m) case v: return m;

#define ENUM_STR_CONVERTER(name, type, enumDef, enumGen) \
    const char *name(type id) {          \
        switch (id) {                    \
            enumDef(enumGen)             \
            default: return "<unknown>"; \
        }                                \
    }

#define ENUM_META_CONVERTER(name, type, enumDef, enumGen, retType, retDefault) \
    retType name(type id) {             \
        switch (id) {                   \
            enumDef(enumGen)            \
            default: return retDefault; \
        }                               \
    }

// Custom colors

#define ASESPRITE_BEIGE CLITERAL(Color){ 210, 203, 190, 255 } // Asesprite Beige
#define FANCY_TIP_YELLOW CLITERAL(Color){ 210, 197, 124, 255 } // Fancy Tip light yellow
#define GRAYISH_BLUE CLITERAL(Color){ 29, 58, 61, 255 } // Some kinda gray-ish blue
#define BLUE_DESAT CLITERAL(Color){ 77, 116, 137, 255 }
#define GREEN_DESAT CLITERAL(Color){ 90, 127, 110, 255 }
#define DIALOG_SEPARATOR_GOLD CLITERAL(Color){ 147, 121, 33, 255 } // Dialog separator color

// Networking

#define PROTOCOL_ID 0x42424242424242ULL

#if 0
#define SV_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_DEBUG
#define CL_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_DEBUG
#else
#define SV_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_INFO
#define CL_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_INFO
#endif

#define FAST_EXIT_SKIP_FREE 1

#define SV_PORT                              4040 //40000
#define SV_TICK_DT                           (1.0/30.0)
#define SV_RENDER                            1
#define SV_MAX_PLAYERS                       8
#define SV_MAX_ENTITIES                      256
#define SV_MAX_ENTITY_NAME_LEN               63 // "Goranza The Arch-Nemesis Defiler of Doom" was the longest name I could think of when I wrote this
#define SV_MAX_ENTITY_SAY_TITLE_LEN          SV_MAX_ENTITY_NAME_LEN
#define SV_MAX_ENTITY_SAY_MSG_LEN            1023
// how long this entity stays interested in a conversation before returning to pathfinding
#define SV_ENTITY_DIALOG_INTERESTED_DURATION 30
#define SV_MAX_TILE_CHUNK_WIDTH              64
#define SV_MAX_TILE_INTERACT_DIST_IN_TILES   1  // max distance player can be from a tile to interact with it
#define SV_MAX_ENTITY_INTERACT_DIST          (TILE_W * 2)  // max distance player can be from a tile to interact with it
#define SV_MAX_TITLE_LEN                     127
#define SV_COMPRESS_TILE_CHUNK_WITH_LZ4      1

//#define CL_PORT                 30000
#define CL_MENU_FADE_IN_DURATION        0.5
#define CL_BANDWIDTH_SMOOTHING_FACTOR   0.99f      // higher = less smooth (thanks yojimbo! -_-)
#define CL_SAMPLE_INPUT_DT              SV_TICK_DT //(1.0/120.0)
#define CL_SEND_INPUT_COUNT             64
#define CL_SEND_INPUT_DT                SV_TICK_DT //(1.0/120.0)
#define CL_SNAPSHOT_COUNT               2
#define CL_RENDER_DISTANCE              1
#define CL_CAMERA_LERP                  0
#define CL_CLIENT_SIDE_PREDICT          1
#define CL_DIALOG_DURATION_MIN          1.0
#define CL_DIALOG_DURATION_PER_CHAR     0.1
#define CL_WARP_FADE_IN_DURATION        0.5  // time it takes in seconds for screen to fade in from black after warping
#define CL_WARP_TITLE_FADE_IN_DELAY     (CL_WARP_FADE_IN_DURATION * 0.5f)  // time to wait before starting to fade in title
#define CL_WARP_TITLE_FADE_IN_DURATION  1.0  // time it takes to fade in title
#define CL_WARP_TITLE_SHOW_DURATION     2.0  // time to show title fully
#define CL_WARP_TITLE_FADE_OUT_DURATION 0.4  // time it takes to fade out title

#define CL_DBG_ONE_SCREEN      0
#define CL_DBG_TWO_SCREEN      0
#define CL_DBG_SNAPSHOT_IDS    0
#define CL_DBG_TILE_CULLING    0
#define CL_DBG_CIRCLE_VS_REC   0
#define CL_DBG_FORCE_ACCUM     0
#define CL_DBG_PIXEL_FIXER     0

Font dlb_LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int glyphCount, int type);
Font dlb_LoadFontEx(const char *fileName, int fontSize, int *fontChars, int glyphCount, int type);
Vector2 dlb_MeasureTextEx(Font font, const char *text, size_t textLen, Vector2 *cursor = 0);
Vector2 dlb_MeasureTextShadowEx(Font font, const char *text, size_t textLen, Vector2 *cursor = 0);
void dlb_DrawTextEx(Font font, const char *text, size_t textLen, Vector2 position, Color tint, Vector2 *cursor = 0, bool *hovered = 0);
void dlb_DrawTextShadowEx(Font font, const char *text, size_t textLen, Vector2 pos, Color color);

float GetRandomFloatZeroToOne(void);
float GetRandomFloatMinusOneToOne(void);
float GetRandomFloatVariance(float variance);

extern Vector2 g_RenderSize;

inline float Vector2CrossProduct(Vector2 a, Vector2 b)
{
    return a.x * b.y - a.y * b.x;
}
inline Vector2 Vector2Floor(Vector2 v)
{
    return { floorf(v.x), floorf(v.y) };
}

Rectangle GetCameraRectWorld(Camera2D &camera);
Rectangle RectShrink(const Rectangle &rect, Vector2 pixels);
Rectangle RectShrink(const Rectangle &rect, float pixels);
Rectangle RectGrow(const Rectangle &rect, Vector2 pixels);
Rectangle RectGrow(const Rectangle &rect, float pixels);
void RectConstrainToRect(Rectangle &rect, Rectangle boundary);
void RectConstrainToScreen(Rectangle &rect, Vector2 *resultOffset = 0);
void CircleConstrainToScreen(Vector2 &center, float radius, Vector2 *resultOffset = 0);

extern std::stack<Rectangle> scissorStack;
void PushScissorRect(Rectangle rect);
Rectangle GetScissorRect(void);
void PopScissorRect(void);

void dlb_DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
void dlb_DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, Color tint);
void dlb_DrawTextureCentered(Texture2D texture, Rectangle source, Rectangle dest, Color tint);
void dlb_DrawTextureCenteredFull(Texture2D texture, Vector2 center, Color tint);
void dlb_DrawNPatch(Rectangle rec);

bool StrFilter(const char *str, const char *filter);

// Protect Against Wrap Sequence comparisons
// https://www.rfc-editor.org/rfc/rfc1323#section-4
template <typename T>
bool paws_greater(T a, T b) {
    static_assert(std::is_unsigned_v<T>, "PAWS comparison requires unsigned integer type");
    const T diff = (a - b);
    const T half_range = (1 << (sizeof(T) * 8 - 1));
    return diff > 0 && diff < half_range;
}

inline void hash_combine_next(std::size_t& seed) {}

template <typename T, typename... Rest>
inline void hash_combine_next(std::size_t& seed, const T& v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine_next(seed, rest...);
}

template <typename T, typename... Rest>
inline size_t hash_combine(const T& v, Rest... rest) {
    size_t hash{};
    hash_combine_next(hash, v, rest...);
    return hash;
}

// These are drawn deferred so we need to store info for later when hovered
struct FancyTextTip {
    Font *font;
    const char *tip;
    size_t tipLen;
};

void dlb_CommonTests(void);
