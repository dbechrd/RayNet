#pragma once
#include "raylib/raylib.h"
#include "yojimbo.h"

// Stuff that probably shouldn't be here

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 900

#define FONT_PATH "resources/Hack-Bold.ttf"
#define FONT_SIZE 20

// Helper macros

#define LERP(a, b, alpha) ((a) + ((b) - (a)) * (alpha))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Networking

#define PROTOCOL_ID             0x42424242424242ULL

#define SV_PORT                 40000
#define SV_TICK_DT              (1.0/20.0)
#define SV_MAX_ENTITIES         128

#define CL_PORT                 30000
#define CL_SEND_INPUT_COUNT     8
#define CL_SEND_INPUT_DT        (1.0/20.0)
#define CL_SNAPSHOT_COUNT       16

#define CL_DBG_SNAPSHOT_SHADOWS 1