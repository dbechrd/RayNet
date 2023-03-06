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

// Networking

#define PROTOCOL_ID                 0x42424242424242ULL

#define SERVER_PORT                 40000
#define SERVER_TICK_DT              (1.0/20.0)

#define CLIENT_PORT                 30000
#define CLIENT_SEND_INPUT_COUNT     16
#define CLIENT_SEND_INPUT_DT        (1.0/60.0)
#define CLIENT_SNAPSHOT_COUNT       16

#define CLIENT_DBG_SNAPSHOT_SHADOWS 1