#pragma once

#include "raylib/raylib.h"
#include "raylib/raymath.h"
#pragma warning(push, 0)
#include "yojimbo.h"
#pragma warning(pop)
#include <stack>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <cctype>

// Stuff that probably shouldn't be here

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define TILE_W 32

#define TODO_LIST_PATH "resources/todo.txt"

#define LEVEL_001 "maps/level1.dat"

// Helper macros

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX((x), (min)), (max)));
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

// Custom colors

#define ASESPRITE_BEIGE CLITERAL(Color){ 210, 203, 190, 255 } // Asesprite Beige
#define GRAYISH_BLUE CLITERAL(Color){ 29, 58, 61, 255 } // Some kinda gray-ish blue

// Networking

#define PROTOCOL_ID 0x42424242424242ULL

#define SV_PORT                     4040 //40000
#define SV_TICK_DT                  (1.0/30.0)
#define SV_MAX_PLAYERS              8
#define SV_MAX_ENTITIES             SV_MAX_PLAYERS + 32
#define SV_MAX_ENTITY_SAY_MSG_LEN   1024

//#define CL_PORT                 30000
#define CL_SAMPLE_INPUT_DT           SV_TICK_DT //(1.0/120.0)
#define CL_SEND_INPUT_COUNT          8
#define CL_SEND_INPUT_DT             SV_TICK_DT //(1.0/120.0)
#define CL_SNAPSHOT_COUNT            16
#define CL_CLIENT_SIDE_PREDICT       1
#define CL_MAX_DIALOGS               32
#define CL_DIALOG_DURATION_MIN       0.5
#define CL_DIALOG_DURATION_PER_CHAR  0.1

#define CL_DBG_ONE_SCREEN      0
#define CL_DBG_TWO_SCREEN      1
#define CL_DBG_SNAPSHOT_IDS    0
#define CL_DBG_TILE_CULLING    0
#define CL_DBG_CIRCLE_VS_REC   0
#define CL_DBG_COLLIDERS       0
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
};

// Dumb stuff that should get a resource manager or wutevs

extern Shader shdSdfText;
extern Font fntHackBold20;
extern Font fntHackBold32;

extern Texture texLily;

extern Sound sndSoftTick;
extern Sound sndHardTick;

Err InitCommon(void);
void FreeCommon(void);