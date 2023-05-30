#include "ui.h"
#include "../audio/audio.h"
#include "../collision.h"
#include "../common.h"

struct HoverHash {
    Vector2 position{};

    HoverHash(void) {};
    HoverHash(Vector2 position) : position(position) {}

    bool Equals(HoverHash &other) {
        return Vector2Equals(position, other.position);
    }
};

UI::UI(Vector2 position, UIStyle style)
{
    this->position = position;
    styleStack.push(style);
}

void UI::PushStyle(UIStyle style)
{
    styleStack.push(style);
}

const UIStyle &UI::GetStyle(void)
{
    assert(styleStack.size());  // did you PopStyle too many times?
    return styleStack.top();
}

void UI::PopStyle(void)
{
    assert(styleStack.size());  // did you PopStyle too many times?
    styleStack.pop();
}

void Align(const UIStyle &style, Vector2 &position, const Vector2 &size)
{
    switch (style.alignH) {
        case TextAlign_Left: {
            break;
        }
        case TextAlign_Center: {
            position.x -= size.x / 2;
            position.y -= size.y / 2;
            break;
        }
        case TextAlign_Right: {
            position.x -= size.x;
            position.y -= size.y;
            break;
        }
    }
}

UIState CalcState(Rectangle &ctrlRect, HoverHash &prevHoverHash)
{
    HoverHash hash{ { ctrlRect.x, ctrlRect.y } };
    bool prevHoveredCtrl = hash.Equals(prevHoverHash);

    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), ctrlRect)) {
        state.entered = !prevHoveredCtrl;
        state.hover = true;
        io.CaptureMouse();
        if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state.pressed = true;
            }
        } else if (io.MouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state.released = true;
        }
        prevHoverHash = hash;
    } else if (prevHoveredCtrl) {
        prevHoverHash = {};
    }
    return state;
}

void UI::UpdateAudio(const UIState &uiState)
{
    if (uiState.released) {
        rnSoundCatalog.Play(STR_SND_HARD_TICK);
    } else if (uiState.entered) {
        rnSoundCatalog.Play(STR_SND_SOFT_TICK, false);
    }
}

void UI::UpdateCursor(const UIStyle &style, Rectangle &ctrlRect)
{
    // How much total space we used up (including margin)
    Vector2 ctrlSpaceUsed{
        style.margin.left + ctrlRect.width + style.margin.right,
        style.margin.top + ctrlRect.height + style.margin.bottom,
    };
    lineSize.x += ctrlSpaceUsed.x;
    lineSize.y = MAX(ctrlSpaceUsed.y, lineSize.y);
    cursor.x += ctrlSpaceUsed.x;
}

void UI::Newline(void)
{
    cursor.x = 0;
    cursor.y += lineSize.y;
    lineSize = {};
}

void UI::Space(Vector2 space)
{
    cursor = Vector2Add(cursor, space);
}

UIState UI::Text(const char *text)
{
    const UIStyle &style = GetStyle();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
    Align(style, ctrlPosition, textSize);
    Vector2 ctrlSize{
        style.pad.left + textSize.x + style.pad.right,
        style.pad.top + textSize.y + style.pad.bottom
    };

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x,
        ctrlSize.y
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    // Draw text
    DrawTextShadowEx(*style.font, text, { ctrlPosition.x, ctrlPosition.y }, style.fgColor);

    state.contentTopLeft = { ctrlRect.x, ctrlRect.y };
    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Text(const char *text, Color fgColor)
{
    UIStyle style = GetStyle();
    style.fgColor = fgColor;
    PushStyle(style);
    UIState state = Text(text);
    PopStyle();
    return state;
}

UIState UI::Image(Texture &texture, Rectangle srcRect)
{
    if (!srcRect.width) {
        srcRect = { 0, 0, (float)texture.width, (float)texture.height };
    }

    const UIStyle &style = GetStyle();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 ctrlSize{
        srcRect.width * style.scale,
        srcRect.height * style.scale
    };

    Align(style, ctrlPosition, ctrlSize);

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.imageBorderThickness * 2,
        ctrlSize.y + style.imageBorderThickness * 2
    };

    Rectangle contentRect = ctrlRect;
    contentRect.x += style.imageBorderThickness;
    contentRect.y += style.imageBorderThickness;
    contentRect.width -= style.imageBorderThickness * 2;
    contentRect.height -= style.imageBorderThickness * 2;

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    // Draw border
    Color borderColor = state.hover ? YELLOW : BLACK;
    if (style.borderColor.a) {
        borderColor = style.borderColor;
    }
    DrawRectangleLinesEx(ctrlRect, style.imageBorderThickness, borderColor);

    // Draw image
    DrawTexturePro(texture, srcRect, contentRect, {}, 0, WHITE);

    state.contentTopLeft = { contentRect.x, contentRect.y };
    UpdateAudio(state);
    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Button(const char *text)
{
    const UIStyle &style = GetStyle();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    const float cornerRoundness = 0.2f;
    const float cornerSegments = 4;

    Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
    Vector2 ctrlSize{
        style.pad.left + textSize.x + style.pad.right,
        style.pad.top + textSize.y + style.pad.bottom
    };

    Align(style, ctrlPosition, ctrlSize);

    Rectangle ctrlRect{
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.buttonBorderThickness * 2,
        ctrlSize.y + style.buttonBorderThickness * 2
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    Color bgColorFx = style.bgColor;
    Color fgColorFx = style.fgColor;
    if (state.hover) {
        bgColorFx = ColorBrightness(style.bgColor, 0.3f);
        // HACK(dlb): We should just make fgColor[state] for hover, down etc.
        // Perhaps with special values that mean "brighten" and "darken".
        if (!bgColorFx.a) fgColorFx = YELLOW;
        if (state.down) {
            bgColorFx = ColorBrightness(style.bgColor, -0.3f);
        }
    }

    // Draw drop shadow
    if (style.bgColor.a) {
        DrawRectangleRounded(ctrlRect, cornerRoundness, cornerSegments, BLACK);
    }

    const float downOffset = (style.buttonPressed || state.down) ? 1 : -1;
    Rectangle contentRect{
        ctrlPosition.x + style.buttonBorderThickness,
        ctrlPosition.y + style.buttonBorderThickness * downOffset,
        ctrlSize.x,
        ctrlSize.y
    };

    // Draw button
    DrawRectangleRounded(contentRect, cornerRoundness, cornerSegments, bgColorFx);

    // Draw button text
    DrawTextShadowEx(*style.font, text,
        {
            contentRect.x + style.pad.left,
            contentRect.y + style.pad.top
        },
        fgColorFx
    );

    state.contentTopLeft = { contentRect.x, contentRect.y };
    UpdateAudio(state);
    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Button(const char *text, Color bgColor)
{
    UIStyle style = GetStyle();
    style.bgColor = bgColor;
    PushStyle(style);
    UIState state = Button(text);
    PopStyle();
    return state;
}

UIState UI::Button(const char *text, bool pressed, Color bgColor, Color bgColorPressed)
{
    UIStyle style = GetStyle();
    style.bgColor = pressed ? bgColorPressed : bgColor;
    style.buttonPressed = pressed;
    PushStyle(style);
    UIState state = Button(text);
    PopStyle();
    return state;
}

struct StbString {
    const Font *font;
    std::string &data;

    StbString(const Font *font, std::string &data)
        : font(font), data(data) {}
};

void RN_stb_layout_row(StbTexteditRow *row, StbString *str, int startIndex)
{
    assert(startIndex == 0); // We're not handling multi-line for now

    Vector2 textSize = MeasureTextEx(*str->font, str->data.c_str() + startIndex, str->font->baseSize, 1);
    assert(textSize.y == str->font->baseSize);

    row->x0 = 0;
    row->x1 = textSize.x;
    row->baseline_y_delta = textSize.y;
    row->ymin = 0;
    row->ymax = str->font->baseSize;
    row->num_chars = str->data.size() - startIndex; // TODO: Word wrap if multi-line is needed
}

int RN_stb_get_char_width(StbString *str, int startIndex, int offset) {
    std::string oneChar = str->data.substr(startIndex + offset, 1);
    Vector2 charSize = MeasureTextEx(*str->font, oneChar.c_str(), str->font->baseSize, 1);
    // NOTE(dlb): Wtf? Raylib probably doing int truncation bullshit.
    return charSize.x + 1;
}

int RN_stb_key_to_char(int key)
{
    if (isascii(key)) {
        return key;
    }
    return -1;
}

int RN_stb_get_char(StbString *str, int index)
{
    return str->data[index];
}

void RN_stb_delete_chars(StbString *str, int index, int count)
{
    str->data.erase(index, count);
}

bool RN_stb_insert_chars(StbString *str, int index, char *chars, int charCount)
{
    str->data.insert(index, chars, charCount);
    return true;
}

bool RN_stb_is_space(char ch)
{
    return isspace(ch);
}

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

UIState UI::Textbox(STB_TexteditState &stbState, std::string &text)
{
    const Vector2 textOffset{ 8, 2 };
    const UIStyle &style = GetStyle();
    StbString str{ style.font, text };

    if (!stbState.initialized) {
        stb_textedit_initialize_state(&stbState, true);
    }

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 textSize = MeasureTextEx(*style.font, text.c_str(), style.font->baseSize, 1.0f);
    Align(style, ctrlPosition, textSize);
    Vector2 ctrlSize{
        style.pad.left + textSize.x + style.pad.right,
        style.pad.top + textSize.y + style.pad.bottom
    };

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x,
        ctrlSize.y
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    static Vector2 mouseClickedAt{};
    static Vector2 mouseDraggedAt{};
    if (state.pressed) {
        Vector2 relMousePos{
            GetMouseX() - (ctrlPosition.x + textOffset.x),
            GetMouseY() - (ctrlPosition.y + textOffset.y)
        };
        stb_textedit_click(&str, &stbState, relMousePos.x, relMousePos.y);
        mouseClickedAt = relMousePos;
    } else if (state.down || state.released) {
        Vector2 relMousePos{
            GetMouseX() - (ctrlPosition.x + textOffset.x),
            GetMouseY() - (ctrlPosition.y + textOffset.y)
        };
        stb_textedit_drag(&str, &stbState, relMousePos.x, relMousePos.y);
        mouseDraggedAt = relMousePos;
    }

    int key = GetKeyPressed();
    while (key) {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            key |= STB_TEXTEDIT_K_CTRL;
        }
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            key |= STB_TEXTEDIT_K_SHIFT;
        }
        if (RN_stb_key_to_char(key) == -1) {
            stb_textedit_key(&str, &stbState, key);
        }
        if (key == (STB_TEXTEDIT_K_CTRL | KEY_A)) {
            stbState.cursor = text.size();
            stbState.select_start = 0;
            stbState.select_end = text.size();
        }
        key = GetKeyPressed();
    }
    int ch = GetCharPressed();
    while (ch) {
        if (RN_stb_key_to_char(key) != -1) {
            stb_textedit_key(&str, &stbState, ch);
        }
        ch = GetCharPressed();
    }

    //    void stb_textedit_drag(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
    //    int  stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
    //    int  stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE *text, int len)

    // Background
    DrawRectangleRec(ctrlRect, DARKGRAY); //BLUE_DESAT);

    // Border
    DrawRectangleLinesEx(ctrlRect, 1, BLACK);

    // Text
    //text.insert(stbState.cursor, "|");
    DrawTextShadowEx(*style.font, text.c_str(), { ctrlPosition.x + textOffset.x, ctrlPosition.y + textOffset.y }, RAYWHITE);
    //text.erase(stbState.cursor, 1);

    if (stbState.select_start != stbState.select_end) {
        int selectLeft = MIN(stbState.select_start, stbState.select_end);
        int selectRight = MAX(stbState.select_start, stbState.select_end);
        float selectOffsetX = 0;
        if (selectLeft) {
            std::string textBeforeSelection = text.substr(0, selectLeft);
            Vector2 textBeforeSelectionSize = MeasureTextEx(*style.font, textBeforeSelection.c_str(), style.font->baseSize, 1);
            selectOffsetX = textBeforeSelectionSize.x + 1;
        }
        std::string selectedText = text.substr(selectLeft, selectRight - selectLeft);
        Vector2 selectedTextSize = MeasureTextEx(*style.font, selectedText.c_str(), style.font->baseSize, 1);
        float selectWidth = selectedTextSize.x;
        Rectangle selectionRect{
            ctrlRect.x + textOffset.x + selectOffsetX,
            ctrlRect.y + textOffset.y,
            selectWidth,
            style.font->baseSize
        };
        DrawRectangleRec(selectionRect, Fade(SKYBLUE, 0.5f));
    }

    std::string textBeforeCursor = text.substr(0, stbState.cursor);
    Vector2 textBeforeCursorSize = MeasureTextEx(*style.font, textBeforeCursor.c_str(), style.font->baseSize, 1);
    Vector2 cursorPos{
        ctrlRect.x + textOffset.x + textBeforeCursorSize.x,
        ctrlRect.y + textOffset.y
    };
    Rectangle cursorRect{
        cursorPos.x,
        cursorPos.y,
        1,
        style.font->baseSize
    };
    //DrawTextEx(*style.font, "|", cursorPos, style.font->baseSize, 1, WHITE);
    DrawRectangleRec(cursorRect, RAYWHITE);

    //if (stbState.cursor < text.size()) {
    //    text[stbState.cursor]
    //}
    //text[stbState.cursor]
    //    text.insert(stbState.cursor, "|");
    //text.erase(stbState.cursor, 1);

    state.contentTopLeft = { ctrlRect.x, ctrlRect.y };
    UpdateCursor(style, ctrlRect);

    Newline();
    Text(TextFormat("clickedAt %.f %.f", mouseClickedAt.x, mouseClickedAt.y));
    Newline();
    Text(TextFormat("draggedAt %.f %.f", mouseDraggedAt.x, mouseDraggedAt.y));
    Newline();
    Text(TextFormat("cursor       %d", stbState.cursor));
    Newline();
    Text(TextFormat("select_start %d", stbState.select_start));
    Newline();
    Text(TextFormat("select_end   %d", stbState.select_end));
    Newline();
    Text(TextFormat("insert       %d", stbState.insert_mode));
    Newline();
    // TODO: Render undo state just for fun?

    return state;
}