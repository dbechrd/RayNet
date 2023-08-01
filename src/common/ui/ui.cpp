#include "ui.h"
#include "../collision.h"
#include "../common.h"

STB_TexteditState *UI::prevActiveEditor{};
STB_TexteditState *UI::activeEditor{};

bool UI::tabToNextEditor{};
STB_TexteditState *UI::lastDrawnEditor{};
STB_TexteditState *UI::tabToPrevEditor{};

bool UI::UnfocusActiveEditor(void)
{
    if (activeEditor) {
        prevActiveEditor = activeEditor;
        activeEditor = 0;
        return true;
    }
    return false;
}

UI::UI(Vector2 position, UIStyle style)
{
    this->position = position;
    styleStack.push(style);
}

void UI::PushStyle(UIStyle style)
{
    styleStack.push(style);
}

void UI::PushMargin(UIMargin margin)
{
    UIStyle style = GetStyle();
    style.margin = margin;
    PushStyle(style);
}

void UI::PushPadding(UIPad padding)
{
    UIStyle style = GetStyle();
    style.pad = padding;
    PushStyle(style);
}

void UI::PushScale(float scale)
{
    UIStyle style = GetStyle();
    style.scale = scale;
    PushStyle(style);
}

void UI::PushWidth(float width)
{
    UIStyle style = GetStyle();
    style.size.x = width;
    PushStyle(style);
}

void UI::PushHeight(float height)
{
    UIStyle style = GetStyle();
    style.size.y = height;
    PushStyle(style);
}

void UI::PushBgColor(Color color, UI_CtrlType ctrlType)
{
    UIStyle style = GetStyle();
    style.bgColor[ctrlType] = color;
    PushStyle(style);
}

void UI::PushFgColor(Color color)
{
    UIStyle style = GetStyle();
    style.fgColor = color;
    PushStyle(style);
}

void UI::PushFont(Font &font)
{
    UIStyle style = GetStyle();
    style.font = &font;
    PushStyle(style);
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

UIState UI::CalcState(Rectangle &ctrlRect, HoverHash &prevHoverHash)
{
    HoverHash hash{ { ctrlRect.x, ctrlRect.y } };
    bool prevHoveredCtrl = hash.Equals(prevHoverHash);

    static HoverHash prevPressedHash{};
    bool prevPressedCtrl = hash.Equals(prevPressedHash);

    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), ctrlRect)) {
        state.entered = !prevHoveredCtrl;
        state.hover = true;
        io.CaptureMouse();
        if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state.pressed = true;
                if (!prevPressedCtrl) {
                    UnfocusActiveEditor();
                }
                prevPressedHash = hash;
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
        data::PlaySound(data::SFX_FILE_SOFT_TICK);
    } else if (uiState.entered) {
        data::PlaySound(data::SFX_FILE_SOFT_TICK, false);
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

    Vector2 contentSize = style.size;
    if (contentSize.x <= 0 || contentSize.y <= 0) {
        Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
        if (contentSize.x <= 0) {
            contentSize.x = textSize.x;
        }
        if (contentSize.y <= 0) {
            contentSize.y = textSize.y;
        }
    }
    Align(style, ctrlPosition, contentSize);
    Vector2 ctrlSize{
        style.pad.left + contentSize.x + style.pad.right,
        style.pad.top + contentSize.y + style.pad.bottom
    };

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x,
        ctrlSize.y
    };

    Vector2 contentPos = {
        ctrlPosition.x + style.pad.left,
        ctrlPosition.y + style.pad.top
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    if (style.bgColor[UI_CtrlTypeDefault].a) {
        DrawRectangleRec(ctrlRect, style.bgColor[UI_CtrlTypeDefault]);
    }
    if (style.borderColor.a) {
        DrawRectangleLinesEx(ctrlRect, 1, style.borderColor);
    }

    // Draw text
    DrawTextShadowEx(*style.font, text, contentPos, style.fgColor);

    state.contentRect = ctrlRect;
    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Text(const char *text, Color fgColor, Color bgColor)
{
    UIStyle style = GetStyle();
    style.bgColor[UI_CtrlTypeDefault] = bgColor;
    style.fgColor = fgColor;
    PushStyle(style);
    UIState state = Text(text);
    PopStyle();
    return state;
}

UIState UI::Label(const char *text, int width)
{
    PushWidth(width);
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

    state.contentRect = contentRect;
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

    Vector2 size = style.size;
    if (!size.x || !size.y) {
        Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
        if (!size.x) size.x = textSize.x;
        if (!size.y) size.y = textSize.y;
    }
    Vector2 ctrlSize{
        style.pad.left + size.x + style.pad.right,
        style.pad.top + size.y + style.pad.bottom
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

    Color bgColorFx = style.bgColor[UI_CtrlTypeButton];
    Color fgColorFx = style.fgColor;
    if (state.hover) {
        bgColorFx = ColorBrightness(bgColorFx, 0.3f);
        // HACK(dlb): We should just make fgColor[state] for hover, down etc.
        // Perhaps with special values that mean "brighten" and "darken".
        if (!bgColorFx.a) {
            fgColorFx = YELLOW;
        }
        if (state.down) {
            bgColorFx = ColorBrightness(bgColorFx, -0.3f);
        }
    }

    // Draw drop shadow
    if (bgColorFx.a) {
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

    state.contentRect = contentRect;
    UpdateAudio(state);
    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Button(const char *text, Color bgColor)
{
    UIStyle style = GetStyle();
    style.bgColor[UI_CtrlTypeButton] = bgColor;
    PushStyle(style);
    UIState state = Button(text);
    PopStyle();
    return state;
}

UIState UI::Button(const char *text, bool pressed, Color bgColor, Color bgColorPressed)
{
    UIStyle style = GetStyle();
    style.bgColor[UI_CtrlTypeButton] = pressed ? bgColorPressed : bgColor;
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
    std::string oneChar = str->data.substr((size_t)startIndex + offset, 1);
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

UIState UI::Textbox(STB_TexteditState &stbState, std::string &text, KeyCallback keyCallback, void *userData)
{
    const Vector2 textOffset{ 8, 2 };
    const UIStyle &style = GetStyle();
    StbString str{ style.font, text };

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 contentSize = style.size;
    if (contentSize.x <= 0 || contentSize.y <= 0) {
        Vector2 textSize = MeasureTextEx(*style.font, text.c_str(), style.font->baseSize, 1.0f);
        if (contentSize.x <= 0) {
            contentSize.x = textSize.x;
        }
        if (contentSize.y <= 0) {
            contentSize.y = textSize.y;
        }
    }
    Align(style, ctrlPosition, contentSize);
    Vector2 ctrlSize{
        style.pad.left + contentSize.x + style.pad.right,
        style.pad.top + contentSize.y + style.pad.bottom
    };

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x,
        ctrlSize.y
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);
    if (state.pressed || tabToNextEditor || &stbState == tabToPrevEditor) {
        prevActiveEditor = activeEditor;
        activeEditor = &stbState;
        tabToNextEditor = false;
        tabToPrevEditor = 0;
    }

    Vector2 mousePosRel{
        GetMouseX() - (ctrlPosition.x + textOffset.x),
        GetMouseY() - (ctrlPosition.y + textOffset.y)
    };

    static double lastClickTime{};
    static Vector2 lastClickMousePosRel{};
    static int mouseClicks{};
    static int dblClickWordLeft{};
    static int dblClickWordRight{};
    static bool newlyActivePress{};

    const bool wasActive = prevActiveEditor == &stbState;
    const bool isActive = activeEditor == &stbState;
    const bool newlyActive = isActive && (!stbState.initialized || !wasActive);

    const double now = GetTime();
    const double timeSinceLastClick = now - lastClickTime;
    if (isActive) {
        if (newlyActive) {
            stb_textedit_initialize_state(&stbState, true);
            prevActiveEditor = activeEditor;
            newlyActivePress = true;
        }

        if (!state.down) {
            newlyActivePress = false;
            if (timeSinceLastClick >= 0.3) {
                mouseClicks = 0;
            }
        }

        if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouseDelta = Vector2Subtract(lastClickMousePosRel, mousePosRel);
            const float maxPixelMovement = 3;
            const float mouseDeltaLen = Vector2Length(mouseDelta);
            if (mouseDeltaLen < maxPixelMovement) {
                mouseClicks++;
            } else {
                // Mouse moved too much
                mouseClicks = 1;
            }
            lastClickTime = now;
            lastClickMousePosRel = mousePosRel;
        }

        if (mouseClicks <= 1) {
            if (state.pressed) {
                stb_textedit_click(&str, &stbState, mousePosRel.x, mousePosRel.y);
            } else if (state.down && !newlyActivePress) {
                stb_textedit_drag(&str, &stbState, mousePosRel.x, mousePosRel.y);
            }
        } else if (mouseClicks == 2) {
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // double click, select word under cursor
                int wordLeft = stbState.cursor;
                while (wordLeft && !RN_stb_is_space(RN_stb_get_char(&str, wordLeft - 1))) {
                    wordLeft--;
                }
                int wordRight = stbState.cursor;
                while (wordRight < STB_TEXTEDIT_STRINGLEN(&str) && !RN_stb_is_space(RN_stb_get_char(&str, wordRight))) {
                    wordRight++;
                }
                if (wordLeft < wordRight) {
                    stbState.select_start = wordLeft;
                    stbState.select_end = wordRight;
                    stbState.cursor = stbState.select_end;
                }

                // store word boundary of word selected when double click first happened
                dblClickWordLeft = wordLeft;
                dblClickWordRight = wordRight;
            } else if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
                // double click drag, select words in range of where mouse was first clicked to where it is now

                // find word boundaries from current mouse position
                stb_textedit_click(&str, &stbState, mousePosRel.x, mousePosRel.y);
                int wordLeft = stbState.cursor;
                while (wordLeft && !RN_stb_is_space(RN_stb_get_char(&str, wordLeft - 1))) {
                    wordLeft--;
                }
                int wordRight = stbState.cursor;
                while (wordRight < STB_TEXTEDIT_STRINGLEN(&str) && !RN_stb_is_space(RN_stb_get_char(&str, wordRight))) {
                    wordRight++;
                }

                // union word boundaries of initial word with current word
                wordLeft = MIN(wordLeft, dblClickWordLeft);
                wordRight = MAX(wordRight, dblClickWordRight);

                if (wordLeft < wordRight) {
                    stbState.select_start = wordLeft;
                    stbState.select_end = wordRight;
                    stbState.cursor = wordLeft < dblClickWordLeft ? stbState.select_start : stbState.select_end;
                }
            }
        } else if (mouseClicks > 2) {
            stbState.select_start = 0;
            stbState.select_end = STB_TEXTEDIT_STRINGLEN(&str);
            stbState.cursor = stbState.select_start;
        }

        if (!io.KeyboardCaptured()) {
            int key = GetKeyPressed();
            while (key) {
                if (keyCallback) {
                    const char *newStr = keyCallback(key, userData);
                    if (newStr) {
                        str.data = newStr;
                        key = GetKeyPressed();
                        continue;
                    }
                }

                if (io.KeyDown(KEY_LEFT_CONTROL) || io.KeyDown(KEY_RIGHT_CONTROL)) {
                    key |= STB_TEXTEDIT_K_CTRL;
                }
                if (io.KeyDown(KEY_LEFT_SHIFT) || io.KeyDown(KEY_RIGHT_SHIFT)) {
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
                if (key == (STB_TEXTEDIT_K_CTRL | KEY_X)) {
                    int selectLeft = MIN(stbState.select_start, stbState.select_end);
                    int selectRight = MAX(stbState.select_start, stbState.select_end);
                    int selectLen = selectRight - selectLeft;
                    if (selectLen) {
                        std::string selectedText = str.data.substr(selectLeft, selectLen);
                        SetClipboardText(selectedText.c_str());
                        stb_textedit_cut(&str, &stbState);
                    }
                }
                if (key == (STB_TEXTEDIT_K_CTRL | KEY_C)) {
                    int selectLeft = MIN(stbState.select_start, stbState.select_end);
                    int selectRight = MAX(stbState.select_start, stbState.select_end);
                    int selectLen = selectRight - selectLeft;
                    if (selectLen) {
                        std::string selectedText = str.data.substr(selectLeft, selectLen);
                        SetClipboardText(selectedText.c_str());
                    }
                }
                if (key == (STB_TEXTEDIT_K_CTRL | KEY_V)) {
                    const char *clipboard = GetClipboardText();
                    if (clipboard) {
                        size_t clipboardLen = strlen(clipboard);
                        if (clipboardLen) {
                            stb_textedit_paste(&str, &stbState, clipboard, clipboardLen);
                        }
                    }
                }
                if (key == KEY_TAB) {
                    // tell next textbox that draws to active
                    tabToNextEditor = true;
                } else if (key == (STB_TEXTEDIT_K_SHIFT | KEY_TAB)) {
                    // tell previously drawn textbox to activate next frame
                    tabToPrevEditor = lastDrawnEditor;
                }
                key = GetKeyPressed();
            }
            int ch = GetCharPressed();
            while (ch) {
                /*if (keyCallback) {
                    const char *newStr = keyCallback(ch, userData);
                    if (newStr) {
                        str.data = newStr;
                        ch = GetCharPressed();
                        continue;
                    }
                }*/

                if (RN_stb_key_to_char(key) != -1) {
                    stb_textedit_key(&str, &stbState, ch);
                }
                ch = GetCharPressed();
            }
        }
    }

    // Background
    Color bgColor = style.bgColor[UI_CtrlTypeDefault];
    if (!bgColor.a) {
        bgColor = DARKGRAY;
    }
    DrawRectangleRec(ctrlRect, bgColor);

    // Border
    DrawRectangleLinesEx(ctrlRect, 1, BLACK);

    // Text
    DrawTextShadowEx(*style.font, text.c_str(), { ctrlPosition.x + textOffset.x, ctrlPosition.y + textOffset.y }, RAYWHITE);

    // Selection highlight
    if (isActive) {
        if (stbState.select_start != stbState.select_end) {
            int selectLeft = MIN(stbState.select_start, stbState.select_end);
            int selectRight = MAX(stbState.select_start, stbState.select_end);
            float selectOffsetX = 0;
            if (selectLeft) {
                std::string textBeforeSelection = text.substr(0, selectLeft);
                Vector2 textBeforeSelectionSize = MeasureTextEx(*style.font, textBeforeSelection.c_str(), style.font->baseSize, 1);
                selectOffsetX = textBeforeSelectionSize.x + 1;
            }
            std::string selectedText = text.substr(selectLeft, (size_t)selectRight - selectLeft);
            Vector2 selectedTextSize = MeasureTextEx(*style.font, selectedText.c_str(), style.font->baseSize, 1);
            float selectWidth = selectedTextSize.x;
            Rectangle selectionRect{
                ctrlRect.x + textOffset.x + selectOffsetX,
                ctrlRect.y + textOffset.y,
                selectWidth,
                (float)style.font->baseSize
            };
            DrawRectangleRec(selectionRect, Fade(SKYBLUE, 0.5f));
        }

        float textBeforeCursorX = 0;
        if (stbState.cursor) {
            std::string textBeforeCursor = text.substr(0, stbState.cursor);
            Vector2 textBeforeCursorSize = MeasureTextEx(*style.font, textBeforeCursor.c_str(), style.font->baseSize, 1);
            textBeforeCursorX = textBeforeCursorSize.x;
        }
        Vector2 cursorPos{
            ctrlRect.x + textOffset.x + textBeforeCursorX,
            ctrlRect.y + textOffset.y
        };
        Rectangle cursorRect{
            cursorPos.x,
            cursorPos.y,
            1,
            (float)style.font->baseSize
        };
        //DrawTextEx(*style.font, "|", cursorPos, style.font->baseSize, 1, WHITE);
        DrawRectangleRec(cursorRect, RAYWHITE);
    }

    state.contentRect = ctrlRect;
    UpdateCursor(style, ctrlRect);

#if 0
    Newline();
    Text(TextFormat("mouseClicks  %d", mouseClicks));
    Newline();
    Text(TextFormat("cursor       %d", stbState.cursor));
    Newline();
    Text(TextFormat("select_start %d", stbState.select_start));
    Newline();
    Text(TextFormat("select_end   %d", stbState.select_end));
    // TODO: Render undo state just for fun?
#endif

    lastDrawnEditor = &stbState;
    return state;
}

struct TextboxFloatCallbackData {
    const char *fmt;
    float *val;
};

const char *AdjustFloat(int key, void *userData)
{
    TextboxFloatCallbackData *data = (TextboxFloatCallbackData *)userData;
    const char *newStr = 0;

    if (key == KEY_UP) {
        *data->val += 1;
        newStr = TextFormat(data->fmt, *data->val);
    }
    if (key == KEY_DOWN) {
        *data->val -= 1;
        newStr = TextFormat(data->fmt, *data->val);
    }

    return newStr;
}

UIState UI::TextboxFloat(STB_TexteditState &stbState, float &value, float width, const char *fmt)
{
#if 0
    const char *valueCstr = TextFormat("%.2f", value);

    if (&stbState != activeEditor) {
        UIStyle clickableText = GetStyle();
        clickableText.bgColor = GRAY;
        PushStyle(clickableText);
        if (Text(valueCstr).pressed) {
            prevActiveEditor = activeEditor;
            activeEditor = &stbState;
        }
        PopStyle();
    } else {
        std::string valueStr{valueCstr};
        UIState state = Textbox(stbState, valueStr);
        char *end = 0;
        float newValue = strtof(valueStr.c_str(), &end);
        if (*end != '\0') {
            // todo: check errors before saving float value
        }
        value = newValue;
        //return state;
    }
#else
    /*if (activeEditor == &stbState) {
        if (io.KeyDown(KEY_UP)) {
            value += 1;
        }
        if (io.KeyDown(KEY_DOWN)) {
            value -= 1;
        }
    }*/

    const char *valueCstr = TextFormat(fmt, value);
    std::string valueStr{valueCstr};
    if (width) PushWidth(width);
    TextboxFloatCallbackData data{ fmt, &value };
    UIState state = Textbox(stbState, valueStr, AdjustFloat, (void *)&data);
    if (width) PopStyle();
    char *end = 0;
    float newValue = strtof(valueStr.c_str(), &end);
    if (*end != '\0') {
        // todo: check errors before saving float value
    }
    value = newValue;
    return state;
#endif
}