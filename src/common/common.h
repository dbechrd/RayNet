#pragma once

#include "raylib/raylib.h"
#include "raylib/raymath.h"

#pragma warning(push, 0)
#include "yojimbo.h"
#pragma warning(pop)

#include "stb_herringbone_wang_tile.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <cctype>

#include <array>
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

#include "strings.h"
#include "ring_buffer.h"

// Stuff that probably shouldn't be here

//#define WINDOW_WIDTH 1920
//#define WINDOW_HEIGHT 1016

#define TEXT_LINE_SPACING 1.0f

#define TILE_W 64
typedef uint8_t Tile;  // TODO: Remove this?

#define TODO_LIST_PATH "todo.txt"

#define PATH_LEN_MAX 1024
#define LEVEL_001  "map_overworld"

// Developer macros

#define DEV_BUILD_PACK_FILE 1

// Helper macros

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX((x), (min)), (max)))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#define STR(x) #x
#define STRSTR(x) STR(x)
#define ENUM_GEN_VALUE(e) e,
#define ENUM_GEN_VALUE_DESC(e, d) e,
#define ENUM_GEN_CASE_RETURN_STR(e) case e: return #e;
#define ENUM_GEN_CASE_RETURN_DESC(e, d) case e: return d;

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
#define SV_MAX_PLAYERS                       8
#define SV_MAX_ENTITIES                      256
#define SV_MAX_ENTITY_SAY_MSG_LEN            1023
#define SV_MAX_ENTITY_NAME_LEN               63 // "Goranza The Arch-Nemesis Defiler of Doom" was the longest name I could think of when I wrote this
#define SV_MAX_TILE_MAP_NAME_LEN             63
#define SV_MAX_SPRITE_NAME_LEN               63
// how long this entity stays interested in a conversation before returning to pathfinding
#define SV_ENTITY_DIALOG_INTERESTED_DURATION 30
#define SV_MAX_TILE_CHUNK_WIDTH              64
#define SV_WARP_FADE_DURATION                1.0  // delay in seconds for fade transition to be black

//#define CL_PORT                 30000
#define CL_BANDWIDTH_SMOOTHING_FACTOR   0.5f
#define CL_SAMPLE_INPUT_DT              SV_TICK_DT //(1.0/120.0)
#define CL_SEND_INPUT_COUNT             32
#define CL_SEND_INPUT_DT                SV_TICK_DT //(1.0/120.0)
#define CL_SNAPSHOT_COUNT               16
#define CL_RENDER_DISTANCE              1
#define CL_CAMERA_LERP                  1
#define CL_CLIENT_SIDE_PREDICT          1
#define CL_DIALOG_DURATION_MIN          1.0
#define CL_DIALOG_DURATION_PER_CHAR     0.1

#define CL_DBG_ONE_SCREEN      0
#define CL_DBG_TWO_SCREEN      0
#define CL_DBG_SNAPSHOT_IDS    0
#define CL_DBG_TILE_CULLING    0
#define CL_DBG_CIRCLE_VS_REC   0
#define CL_DBG_FORCE_ACCUM     0

// Shared types

enum Err {
    RN_SUCCESS          =  0,
    RN_BAD_ALLOC        = -1,
    RN_BAD_MAGIC        = -2,
    RN_BAD_FILE_READ    = -3,
    RN_BAD_FILE_WRITE   = -4,
    RN_INVALID_SIZE     = -5,
    RN_INVALID_PATH     = -6,
    RN_NET_INIT_FAILED  = -7,
    RN_INVALID_ADDRESS  = -8,
    RN_RAYLIB_ERROR     = -9,
    RN_BAD_ID           = -10,
    RN_OUT_OF_BOUNDS    = -11,
};

const char *ErrStr(Err err);

// Dumb stuff that should get a resource manager or wutevs

extern Shader shdSdfText;
extern Shader shdPixelFixer;
extern int    shdPixelFixerScreenSizeUniformLoc;

extern Font fntTiny;
extern Font fntSmall;
extern Font fntMedium;
extern Font fntBig;

Err InitCommon(void);
void FreeCommon(void);
Font dlb_LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int glyphCount, int type);
Font dlb_LoadFontEx(const char *fileName, int fontSize, int *fontChars, int glyphCount, int type);
Vector2 dlb_MeasureTextEx(Font font, const char *text, size_t textLen, Vector2 *cursor = 0);
void dlb_DrawTextEx(Font font, const char *text, size_t textLen, Vector2 position, Color tint, Vector2 *cursor = 0, bool *hovered = 0);

float GetRandomFloatZeroToOne(void);
float GetRandomFloatMinusOneToOne(void);
float GetRandomFloatVariance(float variance);

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color);
Rectangle GetScreenRectWorld(Camera2D &camera);
Rectangle RectShrink(const Rectangle &rect, float pixels);
Rectangle RectGrow(const Rectangle &rect, float pixels);
void RectConstrainToScreen(Rectangle &rect, Vector2 *resultOffset = 0);
void CircleConstrainToScreen(Vector2 &center, float radius, Vector2 *resultOffset = 0);

void dlb_DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
void dlb_DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, Color tint);
void dlb_DrawNPatch(Rectangle rec);

bool StrFilter(const char *str, const char *filter);

// These are drawn deferred so we need to store info for later when hovered
struct FancyTextTip {
    Font *font;
    const char *tip;
    size_t tipLen;
};

#if 0
struct FancyTextNode {
    enum Type {
        TEXT,
        HOVER_TIP,
        DIALOG_OPTION,
    };

    // ALL
    Type type;
    const char *text;
    size_t textLen;
    Vector2 size;
    Vector2 pos;
    //Color color;

    // HOVER_TIP
    const char *tip;
    size_t tipLen;

    // DIALOG_OPTION
    int optionId;
};

struct FancyTextTree {
    std::vector<FancyTextNode> nodes;
};

bool dlb_FancyTextParse(FancyTextTree &tree, const char *text);
#endif

void dlb_CommonTests(void);

//     STB_TEXTEDIT_CHARTYPE             the character type
#define STB_TEXTEDIT_CHARTYPE char

//     STB_TEXTEDIT_POSITIONTYPE         small type that is a valid cursor position
#define STB_TEXTEDIT_POSITIONTYPE int

//     STB_TEXTEDIT_UNDOSTATECOUNT       the number of undo states to allow
#define STB_TEXTEDIT_UNDOSTATECOUNT 99

//     STB_TEXTEDIT_UNDOCHARCOUNT        the number of characters to store in the undo buffer
#define STB_TEXTEDIT_UNDOCHARCOUNT 999

//    STB_TEXTEDIT_STRING                the type of object representing a string being edited,
//                                       typically this is a wrapper object with other data you need
#define STB_TEXTEDIT_STRING StbString

//    STB_TEXTEDIT_STRINGLEN(obj)        the length of the string (ideally O(1))
#define STB_TEXTEDIT_STRINGLEN(obj) ((int)(obj)->data.size())

//    STB_TEXTEDIT_LAYOUTROW(&r,obj,n)   returns the results of laying out a line of characters
//                                       starting from character #n (see discussion below)
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n) RN_stb_layout_row(r, obj, n)

//    STB_TEXTEDIT_GETWIDTH(obj,n,i)     returns the pixel delta from the xpos of the i'th character
//                                       to the xpos of the i+1'th char for a line of characters
//                                       starting at character #n (i.e. accounts for kerning
//                                       with previous char)
#define STB_TEXTEDIT_GETWIDTH(obj, n, i) RN_stb_get_char_width(obj, n, i)

//    STB_TEXTEDIT_KEYTOTEXT(k)          maps a keyboard input to an insertable character
//                                       (return type is int, -1 means not valid to insert)
#define STB_TEXTEDIT_KEYTOTEXT(k) RN_stb_key_to_char(k)

//    STB_TEXTEDIT_GETCHAR(obj,i)        returns the i'th character of obj, 0-based
#define STB_TEXTEDIT_GETCHAR(obj, i) RN_stb_get_char(obj, i)

//    STB_TEXTEDIT_NEWLINE               the character returned by _GETCHAR() we recognize
//                                       as manually wordwrapping for end-of-line positioning
#define STB_TEXTEDIT_NEWLINE '\n'

//
//    STB_TEXTEDIT_DELETECHARS(obj,i,n)      delete n characters starting at i
#define STB_TEXTEDIT_DELETECHARS(obj, i, n) RN_stb_delete_chars(obj, i, n)

//    STB_TEXTEDIT_INSERTCHARS(obj,i,c*,n)   insert n characters at i (pointed to by STB_TEXTEDIT_CHARTYPE*)
#define STB_TEXTEDIT_INSERTCHARS(obj, i, c, n) RN_stb_insert_chars(obj, i, c, n)

// Custom bitflags for modifier keys
#define STB_TEXTEDIT_K_CTRL      512
#define STB_TEXTEDIT_K_SHIFT     1024

#define STB_TEXTEDIT_K_LEFT      KEY_LEFT
#define STB_TEXTEDIT_K_RIGHT     KEY_RIGHT
#define STB_TEXTEDIT_K_UP        KEY_UP
#define STB_TEXTEDIT_K_DOWN      KEY_DOWN
#define STB_TEXTEDIT_K_DELETE    KEY_DELETE
#define STB_TEXTEDIT_K_BACKSPACE KEY_BACKSPACE
#define STB_TEXTEDIT_K_LINESTART KEY_HOME
#define STB_TEXTEDIT_K_LINEEND   KEY_END
#define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_CTRL | KEY_HOME)
#define STB_TEXTEDIT_K_TEXTEND   (STB_TEXTEDIT_K_CTRL | KEY_END)
#define STB_TEXTEDIT_K_PGUP      KEY_PAGE_UP
#define STB_TEXTEDIT_K_PGDOWN    KEY_PAGE_DOWN
#define STB_TEXTEDIT_K_UNDO      (STB_TEXTEDIT_K_CTRL | KEY_Z)
#define STB_TEXTEDIT_K_REDO      (STB_TEXTEDIT_K_CTRL | STB_TEXTEDIT_K_SHIFT | KEY_Z)

////////////////////////////////////////
// Optional
#define STB_TEXTEDIT_K_INSERT    KEY_INSERT

//    STB_TEXTEDIT_IS_SPACE(ch)          true if character is whitespace (e.g. 'isspace'),
//                                       required for default WORDLEFT/WORDRIGHT handlers
#define STB_TEXTEDIT_IS_SPACE(ch) RN_stb_is_space(ch)

//    STB_TEXTEDIT_MOVEWORDLEFT(obj,i)   custom handler for WORDLEFT, returns index to move cursor to
// default

//    STB_TEXTEDIT_MOVEWORDRIGHT(obj,i)  custom handler for WORDRIGHT, returns index to move cursor to
// default

//    STB_TEXTEDIT_K_WORDLEFT            keyboard input to move cursor left one word // e.g. ctrl-LEFT
#define STB_TEXTEDIT_K_WORDLEFT  (STB_TEXTEDIT_K_CTRL | KEY_LEFT)

//    STB_TEXTEDIT_K_WORDRIGHT           keyboard input to move cursor right one word // e.g. ctrl-RIGHT
#define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_CTRL | KEY_RIGHT)

//    STB_TEXTEDIT_K_LINESTART2          secondary keyboard input to move cursor to start of line
//    STB_TEXTEDIT_K_LINEEND2            secondary keyboard input to move cursor to end of line
//    STB_TEXTEDIT_K_TEXTSTART2          secondary keyboard input to move cursor to start of text
//    STB_TEXTEDIT_K_TEXTEND2            secondary keyboard input to move cursor to end of text
////////////////////////////////////////
#include "stb_textedit.h"
