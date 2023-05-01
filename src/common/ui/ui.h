#pragma once
#include "../common.h"

struct UIState {
    bool hover;
    bool down;
    bool clicked;
};

UIState UIButton(Font font, Color color, const char *text, Vector2 uiPosition, Vector2 &uiCursor);