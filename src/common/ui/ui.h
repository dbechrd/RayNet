#pragma once
#include "../common.h"

struct UIPad {
    float top    {};
    float right  {};
    float bottom {};
    float left   {};

    UIPad() {}

    UIPad(float pad) {
        top     = pad;
        right   = pad;
        bottom  = pad;
        left    = pad;
    }

    UIPad(float x, float y) {
        top     = y;
        right   = x;
        bottom  = y;
        left    = x;
    }

    UIPad(float top, float right, float bottom, float left) {
        this->top     = top;
        this->right   = right;
        this->bottom  = bottom;
        this->left    = left;
    }

    Vector2 TopLeft(void) {
        return { top, left };
    }
};
typedef UIPad UIMargin;

enum UIStyle_TextAlignH {
    TextAlign_Left,
    TextAlign_Center,
    TextAlign_Right,
};

#if 0
enum UIElementType {
    UIElement_Button,
    UIElement_Slider,

    UIElement_Count,
};
#endif

struct UIStyle {
    UIMargin margin{ 6, 0, 0, 6 };
    Vector2 borderThickness{ 1.0f, 1.0f };
    UIPad pad{ 8, 2 };
    Color bgColor{ BLUE };
    Color fgColor{ WHITE };
    Font *font{ &fntHackBold20 };
    UIStyle_TextAlignH alignH{ TextAlign_Left };
};

struct UIState {
    bool hover;
    bool pressed;
    bool down;
    bool clicked;
};

struct UI {
    UI(Vector2 position, UIStyle style);
    void PushStyle(UIStyle style);
    void PopStyle(void);

    void Newline(void);

    void Text(const char *text);
    UIState Button(const char *text);
    UIState Button(const char *text, Color bgColor);

    inline Vector2 CursorScreen(void) {
        return Vector2Add(position, cursor);
    }

private:
    Vector2 position{}; // top left of this UI
    Vector2 cursor{};   // where to draw next element
    Vector2 lineSize{}; // total size of current row of UI elements
    std::stack<UIStyle> styleStack{};
};