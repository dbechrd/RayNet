#pragma once
#include "common.h"
#include "audio/audio.h"
#include "collision.h"
#include "editor.h"
#include "entity.h"
#include "file_utils.h"
#include "input_command.h"
#include "net/entity_snapshot.h"
#include "net/messages/msg_c_input_commands.h"
#include "net/messages/msg_s_clock_sync.h"
#include "net/messages/msg_s_entity_snapshot.h"
#include "net/net.h"
#include "ring_buffer.h"
#include "tilemap.h"
#include "wang.h"

float GetRandomFloatZeroToOne(void);
float GetRandomFloatMinusOneToOne(void);
float GetRandomFloatVariance(float variance);

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, Color color);
Rectangle GetScreenRectWorld(Camera2D &camera);