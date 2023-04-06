#pragma once
#include "common.h"
#include "entity.h"
#include "input_command.h"
#include "net/entity_snapshot.h"
#include "net/messages/msg_c_input_commands.h"
#include "net/messages/msg_s_clock_sync.h"
#include "net/messages/msg_s_entity_snapshot.h"
#include "net/net.h"
#include "ring_buffer.h"

void DrawTextShadowEx(Font font, const char *text, Vector2 pos, float fontSize, Color color);