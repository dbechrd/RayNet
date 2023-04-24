#pragma once
#include "common.h"

bool dlb_CheckCollisionPointRec(Vector2 point, Rectangle rec);
bool dlb_CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec, Vector2 &contact, Vector2 &normal, float &depth);