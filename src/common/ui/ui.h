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
    float scale{ 1 };
    Color bgColor{ BLUE };
    Color fgColor{ WHITE };
    Font *font{ &fntHackBold20 };
    bool buttonPressed{};
    UIStyle_TextAlignH alignH{ TextAlign_Left };
};

struct UIState {
    bool entered;  // first hover frame
    bool hover;
    bool pressed;  // first down frame
    bool down;
    bool clicked;  // frame after last down frame (released)
};

struct UI {
    UI(Vector2 position, UIStyle style);
    void PushStyle(UIStyle style);
    UIStyle GetStyle(void);
    void PopStyle(void);

    void UpdateCursor(const UIStyle &style, Rectangle &ctrlRect);
    void Newline(void);

    void Space(Vector2 space);
    UIState Text(const char *text);
    UIState Text(const char *text, Color fgColor);
    UIState Image(Texture &texture, Rectangle srcRect = {});
    UIState Button(const char *text);
    UIState Button(const char *text, Color bgColor);
    UIState Button(const char *text, bool pressed, Color pressedColor);

    inline Vector2 CursorScreen(void) {
        return Vector2Add(position, cursor);
    }

    void SetCursor(Vector2 cursor) {
        this->cursor = cursor;
    }

private:
    Vector2 position{}; // top left of this UI
    Vector2 cursor{};   // where to draw next element
    Vector2 lineSize{}; // total size of current row of UI elements
    std::stack<UIStyle> styleStack{};
};