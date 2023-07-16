#pragma once
#include "../common.h"

enum UI_CtrlType {
    UI_CtrlTypeDefault,
    UI_CtrlTypeButton,
    UI_CtrlTypeCount,
};

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

    Vector2 TopLeft(void) const {
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
    float buttonBorderThickness{ 1 };
    float imageBorderThickness{ 2 };
    Color borderColor{ BLANK };
    UIPad pad{ 8, 2 };
    float scale{ 1 };
    Vector2 size{ 0, 0 };
    Color bgColor[UI_CtrlTypeCount]{ BLANK, BLUE };
    Color fgColor{ WHITE };
    Font *font{ &fntSmall };
    bool buttonPressed{};
    UIStyle_TextAlignH alignH{ TextAlign_Left };
};

struct UIState {
    bool entered;   // mouse hovering control (first frame)
    bool hover;     // mouse hovering control
    bool pressed;   // mouse hovering and button down (first frame)
    bool down;      // mouse hovering and button down
    bool released;  // mouse hovering and button up (first frame, i.e. was down last frame)
    Rectangle contentRect;
};

struct UI {
    static bool UnfocusActiveEditor(void);

    UI(Vector2 position, UIStyle style);
    void PushStyle(UIStyle style);
    void PushMargin(UIMargin margin);
    void PushPadding(UIPad padding);
    void PushScale(float scale);
    void PushWidth(float width);
    void PushHeight(float height);
    void PushBgColor(Color color, UI_CtrlType ctrlType);
    void PushFgColor(Color color);
    void PushFont(Font &font);
    const UIStyle &GetStyle(void);
    void PopStyle(void);

    void Newline(void);
    void Space(Vector2 space);
    UIState Text(const char *text);
    UIState Text(const char *text, Color fgColor, Color bgColor = BLANK);
    UIState Image(Texture &texture, Rectangle srcRect = {});
    UIState Button(const char *text);
    UIState Button(const char *text, Color bgColor);
    UIState Button(const char *text, bool pressed, Color bgColor, Color bgColorPressed);
    UIState Textbox(STB_TexteditState &state, std::string &text);
    void TextboxFloat(STB_TexteditState &stbState, float &value);

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
    static STB_TexteditState *prevActiveEditor;
    static STB_TexteditState *activeEditor;

    void UpdateAudio(const UIState &uiState);
    void UpdateCursor(const UIStyle &style, Rectangle &ctrlRect);
};
