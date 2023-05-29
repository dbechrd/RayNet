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
#include <thread>
#include <unordered_map>
#include <vector>

// Stuff that probably shouldn't be here

//#define WINDOW_WIDTH 1920
//#define WINDOW_HEIGHT 1016

#define TILE_W 32

#define TODO_LIST_PATH "resources/todo.txt"

#define PATH_LEN_MAX 1024
#define LEVEL_001 "maps/level1.dat"
#define LEVEL_002 "maps/level2.dat"

// Helper macros

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX((x), (min)), (max)));
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

// Custom colors

#define ASESPRITE_BEIGE CLITERAL(Color){ 210, 203, 190, 255 } // Asesprite Beige
#define GRAYISH_BLUE CLITERAL(Color){ 29, 58, 61, 255 } // Some kinda gray-ish blue
#define BLUE_DESAT CLITERAL(Color){ 77, 116, 137, 255 }
#define GREEN_DESAT CLITERAL(Color){ 90, 127, 110, 255 }

// Networking

#define PROTOCOL_ID 0x42424242424242ULL

#if 0
#define SV_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_DEBUG
#define CL_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_DEBUG
#else
#define SV_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_INFO
#define CL_YJ_LOG_LEVEL             YOJIMBO_LOG_LEVEL_INFO
#endif

#define SV_PORT                     4040 //40000
#define SV_TICK_DT                  (1.0/30.0)
#define SV_MAX_PLAYERS              8
#define SV_MAX_ENTITIES             256
#define SV_MAX_ENTITY_SAY_MSG_LEN   1024
#define SV_TILE_CHUNK_WIDTH         64

//#define CL_PORT                 30000
#define CL_BANDWIDTH_SMOOTHING_FACTOR   0.95f
#define CL_SAMPLE_INPUT_DT              SV_TICK_DT //(1.0/120.0)
#define CL_SEND_INPUT_COUNT             32
#define CL_SEND_INPUT_DT                SV_TICK_DT //(1.0/120.0)
#define CL_SNAPSHOT_COUNT               16
#define CL_RENDER_DISTANCE              1
#define CL_CAMERA_LERP                  0
#define CL_CLIENT_SIDE_PREDICT          1
#define CL_DIALOG_DURATION_MIN          0.5
#define CL_DIALOG_DURATION_PER_CHAR     0.1

#define CL_DBG_ONE_SCREEN      0
#define CL_DBG_TWO_SCREEN      1
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
extern Font fntHackBold20;
extern Font fntHackBold32;

extern Texture texLily;

extern Music musAmbientOutdoors;
extern Music musCave;

Err InitCommon(void);
void FreeCommon(void);
Font dlb_LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int glyphCount, int type);
Font dlb_LoadFontEx(const char *fileName, int fontSize, int *fontChars, int glyphCount, int type);

float GetRandomFloatZeroToOne(void);
float GetRandomFloatMinusOneToOne(void);
float GetRandomFloatVariance(float variance);

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color);
Rectangle GetScreenRectWorld(Camera2D &camera);
Rectangle RectShrink(const Rectangle &rect, float pixels);
Rectangle RectGrow(const Rectangle &rect, float pixels);

// TODO: MOOOOOOOOOOOOOOOOOOOOOOOVE THIS
struct IO {
    // In order of least to greatest I/O precedence
    enum Scope {
        IO_None,
        IO_Game,
        IO_GameHUD,
        IO_Editor,
        IO_EditorGroundOverlay,
        IO_EditorEntityOverlay,
        IO_EditorUI,
        IO_EditorDrag,
        IO_F3Menu,
        IO_Count
    };

    void EndFrame(void)
    {
        assert(scopeStack.empty());  // forgot to close a scope?

        prevKeyboardCaptureScope = keyboardCaptureScope;
        prevMouseCaptureScope = mouseCaptureScope;
        keyboardCaptureScope = IO_None;
        mouseCaptureScope = IO_None;
    }

    void PushScope(Scope scope)
    {
        scopeStack.push(scope);
    }

    void PopScope(void)
    {
        scopeStack.pop();
    }

    // TODO(dlb): Maybe these can just be bools in PushScope?
    void CaptureKeyboard(void)
    {
        keyboardCaptureScope = MAX(keyboardCaptureScope, scopeStack.top());
    }

    void CaptureMouse(void)
    {
        mouseCaptureScope = MAX(mouseCaptureScope, scopeStack.top());
    }

    // returns true if keyboard is captured by a higher precedence scope
    bool KeyboardCaptured(void)
    {
        int captureScope = MAX(prevKeyboardCaptureScope, keyboardCaptureScope);
        return captureScope > scopeStack.top();
    }

    // returns true if mouse is captured by a higher precedence scope
    bool MouseCaptured(void)
    {
        int captureScope = MAX(prevMouseCaptureScope, mouseCaptureScope);
        return captureScope > scopeStack.top();
    }

    bool KeyPressed(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyPressed(key);
    }
    bool KeyDown(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyDown(key);
    }
    bool KeyReleased(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyReleased(key);
    }

    bool MouseButtonPressed(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonPressed(button);
    }
    bool MouseButtonDown(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonDown(button);
    }
    bool MouseButtonReleased(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonReleased(button);
    }
    float MouseWheelMove(void) {
        if (MouseCaptured()) {
            return 0;
        }
        return GetMouseWheelMove();
    }

private:
    // NOTE: This default value prevents all input on the first frame (as opposed
    // to letting multiple things handle the input at once) but it probably doesn't
    // matter either way.
    Scope prevKeyboardCaptureScope = IO_Count;
    Scope prevMouseCaptureScope = IO_Count;
    Scope keyboardCaptureScope{};
    Scope mouseCaptureScope{};

    std::stack<Scope> scopeStack;
};

extern IO io;