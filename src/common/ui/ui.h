#pragma once
#include "../data.h"

enum UI_CtrlType {
    UI_CtrlTypeDefault,
    UI_CtrlTypeButton,
    UI_CtrlTypeTextbox,
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
    UIMargin margin{ 4, 0, 0, 4 };
    float buttonBorderThickness{ 1 };
    float imageBorderThickness{ 2 };
    Color borderColor{ BLANK };
    UIPad pad{ 8, 2 };
    float scale{ 1 };
    Vector2 size{ 0, 0 };
    Color bgColor[UI_CtrlTypeCount]{ BLANK, BLUE_DESAT, DARKGRAY };
    Color fgColor{ WHITE };
    Font *font{ &fntSmall };
    bool buttonPressed{};
    UIStyle_TextAlignH alignH{ TextAlign_Left };
    int indent{ 0 };
    float indentSize{ 0 };
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
    static bool IsActiveEditor(uint32_t ctrlid);
    static bool UnfocusActiveEditor(void);

    UI(Vector2 position, UIStyle style);

    inline Vector2 CursorScreen(void) {
        return Vector2Add(position, cursor);
    }

    void SetCursor(Vector2 cursor) {
        this->cursor = cursor;
    }

    void PushStyle(UIStyle style);
    void PushMargin(UIMargin margin);
    void PushPadding(UIPad padding);
    void PushScale(float scale);
    void PushWidth(float width);
    void PushHeight(float height);
    void PushBgColor(Color color, UI_CtrlType ctrlType);
    void PushFgColor(Color color);
    void PushFont(Font &font);
    void PushIndent(int indent = 1);
    const UIStyle &GetStyle(void);
    void PopStyle(void);

    void Newline(void);
    void Space(Vector2 space);
    UIState Text(const char *text, size_t textLen = 0);
    UIState Text(const std::string &text, Color fgColor = BLANK, Color bgColor = BLANK);
    UIState Text(uint8_t value);
    UIState Text(uint16_t value);
    UIState Text(uint32_t value);
    UIState Text(uint64_t value);
    template <typename T> UIState Text(T value);
    UIState Label(const std::string &text, int width = 0);
    UIState Image(const Texture &texture, Rectangle srcRect = {});
    UIState Button(const std::string &text);
    UIState Button(const std::string &text, Color bgColor);
    UIState Button(const std::string &text, bool pressed, Color bgColor, Color bgColorPressed);

    typedef void (*KeyPreCallback)(std::string &str, void *userData, bool &keyHandled);
    typedef void (*KeyPostCallback)(std::string &str, void *userData);

    UIState Textbox(uint32_t ctrlid, std::string &text, bool multline = false, KeyPreCallback preCallback = 0, KeyPostCallback postCallback = 0, void *userData = 0);
    UIState Textbox(uint32_t ctrlid, float &value, const char *fmt = "%.f", float increment = 1);

    template <typename T>
    void HAQField(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth);

    void DrawTooltips(void);
private:
    Vector2 position{}; // top left of this UI
    Vector2 cursor{};   // where to draw next element
    Vector2 lineSize{}; // total size of current row of UI elements
    std::stack<UIStyle> styleStack{};
    static uint32_t prevActiveEditor;
    static uint32_t activeEditor;

    static bool tabToNextEditor;  // for tab
    static bool tabHandledThisFrame;  // to de-dupe
    static uint32_t lastDrawnEditor;  // for shift-tab (1 frame delay)
    static uint32_t tabToPrevEditor;  // for shift-tab (1 frame delay)

    DrawCmdQueue tooltips;

    struct HoverHash {
        Vector2 position{};

        HoverHash(void) {};
        HoverHash(Vector2 position) : position(position) {}

        bool Equals(HoverHash &other) {
            return Vector2Equals(position, other.position);
        }
    };

    UIState CalcState(Rectangle &ctrlRect, HoverHash &prevHoverHash);
    void UpdateAudio(const UIState &uiState);
    void UpdateCursor(const UIStyle &style, Rectangle &ctrlRect);

    template <typename T>
    void HAQFieldValue(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth);
    void HAQFieldValue(uint32_t ctrlid, const std::string &name, ObjectData &obj, int flags, int labelWidth);
    template <typename T>
    void HAQFieldValue(uint32_t ctrlid, const std::string &name, std::vector<T> &vec, int flags, int labelWidth);
    template <typename T>
    void HAQFieldEditor(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth);
    void HAQFieldEditor(uint32_t ctrlid, const std::string &name, TileDef::Flags &value, int flags, int labelWidth);
    void HAQFieldEditor(uint32_t ctrlid, const std::string &name, ObjType &value, int flags, int labelWidth);
};

extern void LimitStringLength(std::string &str, void *userData);