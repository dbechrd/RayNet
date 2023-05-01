#pragma once
#include "../common.h"

extern Sound sndSoftTick;
extern Sound sndHardTick;

struct UIState {
    bool hover;
    bool down;
    bool clicked;
};

Err InitUI(void);
void FreeUI(void);
UIState UIButton(Font font, Color color, const char *text, Vector2 uiPosition, Vector2 &uiCursor);