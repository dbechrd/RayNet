#include "ui.h"
#include "io.h"
#include "../collision.h"
#include "../common.h"

uint32_t UI::prevActiveEditor{};
uint32_t UI::activeEditor{};

bool UI::tabToNextEditor{};
bool UI::tabHandledThisFrame{};
uint32_t UI::lastDrawnEditor{};
uint32_t UI::tabToPrevEditor{};

ScrollPanel *UI::dragPanel{};
Vector2 UI::dragPanelOffset{};
DragMode UI::dragPanelMode{};

DrawCmdQueue UI::tooltips{};
Texture UI::checkerTex{};

bool UI::IsActiveEditor(uint32_t ctrlid)
{
    return ctrlid == activeEditor;
}
bool UI::UnfocusActiveEditor(void)
{
    if (activeEditor) {
        prevActiveEditor = activeEditor;
        activeEditor = 0;
        return true;
    }
    return false;
}

UI::UI(Vector2 &position, Vector2 &size, UIStyle style) : position(position), size(size)
{
    if (!checkerTex.width) {
        const float alpha = 1.0f;
        ::Image checkerImg = GenImageChecked(16, 16, 8, 8, Fade(GRAY, alpha), Fade(LIGHTGRAY, alpha));
        // NOTE(dlb): Intentional memory leak, maybe clean it up one day by moving this to common Init() or wutevs
        checkerTex = LoadTextureFromImage(checkerImg);
        UnloadImage(checkerImg);
    }

    //cursor = { style.pad.left, style.pad.top };
    styleStack.push(style);
    tabHandledThisFrame = false;
}

const UIStyle &UI::GetStyle(void)
{
    assert(styleStack.size());  // did you PopStyle too many times?
    return styleStack.top();
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
void UI::PushSize(Vector2 size)
{
    UIStyle style = GetStyle();
    style.size = size;
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
void UI::PushIndent(int indent)
{
    UIStyle style = GetStyle();
    style.indent += indent;
    PushStyle(style);
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

UIState UI::CalcState(const Rectangle &ctrlRect, HoverHash &prevHoverHash)
{
    HoverHash hash{ { ctrlRect.x, ctrlRect.y } };
    bool prevHoveredCtrl = hash.Equals(prevHoverHash);

    static HoverHash prevPressedHash{};
    bool prevPressedCtrl = hash.Equals(prevPressedHash);

    UIState state{};
    state.ctrlRect = ctrlRect;

    Rectangle scissorRect = ctrlRect;
    RectConstrainToRect(scissorRect, GetScissorRect());

    if (dlb_CheckCollisionPointRec(GetMousePosition(), scissorRect)) {
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
        //PlaySound("sfx_soft_tick");
    } else if (uiState.entered) {
        PlaySound("sfx_soft_tick", false);
    }
}
void UI::UpdateCursor(const Rectangle &ctrlRect)
{
    const UIStyle &style = GetStyle();

    // How much total space we used up (including margin)
    Vector2 ctrlSpaceUsed{
        style.margin.left + ctrlRect.width + style.margin.right,
        style.margin.top + ctrlRect.height + style.margin.bottom,
    };
    lineSize.x += ctrlSpaceUsed.x;
    lineSize.y = MAX(ctrlSpaceUsed.y, lineSize.y);
    cursor.x += ctrlSpaceUsed.x;

    if (panelStack.size()) {
        ScrollPanel &panel = *panelStack.back();
        panel.maxCursor.x = MAX(panel.maxCursor.x, cursor.x + floorf(panel.scrollOffset.x));
        panel.maxCursor.y = MAX(panel.maxCursor.y, cursor.y + floorf(panel.scrollOffset.y) + lineSize.y);
    }
}
bool UI::ShouldCull(const Rectangle &ctrlRect)
{
    // NOTE: This is broken, srollsbars don't calculate content height correctly when culling
#if 1
    return false;
#else
    if (CheckCollisionRecs(ctrlRect, GetScissorRect())) {
        return false;
    }
    return true;
#endif
}

void UI::Newline(void)
{
    const UIStyle &style = GetStyle();

    cursor.x = 0;
    if (panelStack.size()) {
        for (ScrollPanel *panel : panelStack) {
            cursor.x += panel->style.margin.left + panel->style.borderWidth[UI_CtrlTypePanel] + panel->style.pad.left;
            cursor.x -= floorf(panel->scrollOffset.x);
        }
    }

    cursor.y += lineSize.y;
    lineSize = {};

    float indentSize = style.indentSize;
    if (indentSize == 0) {
        indentSize = style.font->baseSize;
    }
    Space({ (float)style.indent * indentSize, 0 });
}
void UI::Space(Vector2 space)
{
    cursor = Vector2Add(cursor, space);
}

void UI::BeginScrollPanel(ScrollPanel &scrollPanel, IO::Scope scope)
{
    panelStack.push_back(&scrollPanel);
    io.PushScope(scope);
    scrollPanel.scope = scope;

    const UIStyle &style = GetStyle();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 ctrlSize{
        style.size.x * style.scale,
        style.size.y * style.scale
    };

    Align(style, ctrlPosition, ctrlSize);

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x,
        ctrlSize.y
    };

    // This sorta works for the first child, but requires a stack to work fully.
    //cursor.x += style.margin.left;
    //cursor.y += style.margin.top;

    Rectangle contentRect = RectShrink(ctrlRect, style.borderWidth[UI_CtrlTypePanel]);

    static HoverHash prevHoverHash{};
    scrollPanel.style = style;
    scrollPanel.state = CalcState(ctrlRect, prevHoverHash);
    scrollPanel.state.contentRect = contentRect;
    scrollPanel.maxCursor = {};

    scrollPanel.wasCulled = ShouldCull(ctrlRect);
    if (scrollPanel.wasCulled) {
        return;
    }

    // Draw background color
    DrawRectangleRec(ctrlRect, style.bgColor[UI_CtrlTypePanel]);
    DrawRectangleLinesEx(ctrlRect, style.borderWidth[UI_CtrlTypePanel], style.borderColor[UI_CtrlTypePanel]);

    Space({
        // NOTE: Newline() calculates x because of scrollOffset complexity
        0,  // style.margin.left + style.panelBorderWidth + style.pad.left,
        style.margin.top + style.borderWidth[UI_CtrlTypePanel] + style.pad.top - floorf(scrollPanel.scrollOffset.y)
    });
    Newline();

#if !DBG_UI_NO_SCISSOR
    PushScissorRect(scrollPanel.state.contentRect);
#endif
}
void UI::EndScrollPanel(ScrollPanel &scrollPanel)
{
    panelStack.pop_back();

    if (scrollPanel.wasCulled) {
        io.PopScope();
        return;
    }

#if !DBG_UI_NO_SCISSOR
    PopScissorRect();
#endif

    Space({
        floorf(scrollPanel.scrollOffset.x),
        floorf(scrollPanel.scrollOffset.y)
    });

    Vector2 &scrollOffsetTarget = scrollPanel.scrollOffsetTarget;
    Vector2 &scrollOffset = scrollPanel.scrollOffset;
    Vector2 &scrollVelocity = scrollPanel.scrollVelocity;

    const Rectangle &panelRect = scrollPanel.state.contentRect;

    const Vector2 fullSize{
        MAX(0, scrollPanel.maxCursor.x - (scrollPanel.scope == IO::IO_ScrollPanelInner ? panelRect.x : 0)),
        MAX(0, scrollPanel.maxCursor.y - panelRect.y)
    };

    const Vector2 maxScrollOffset{
        MAX(0, fullSize.x - panelRect.width ),
        MAX(0, fullSize.y - panelRect.height)
    };

    const float scrollHandleThickness = 8.0f;
    const float scrollHandleFade = 0.3f;

    const bool showScrollX = maxScrollOffset.x > 0;
    const bool showScrollY = maxScrollOffset.y > 0;

    if (showScrollX) {
        if (scrollPanel.state.hover) {
            const float wheel = io.KeyDown(KEY_LEFT_SHIFT) ? io.MouseWheelMove() : 0;
            if (wheel) {
                float impulse = 4 * fabsf(wheel);
                impulse *= impulse;
                if (wheel < 0) impulse *= -1;
                scrollVelocity.x += impulse;
                //printf("wheel: %f, target: %f\n", wheel, scrollOffsetTarget);
            }
        }

        if (scrollVelocity.x) {
            scrollOffsetTarget.x -= scrollVelocity.x;  // * dt
            scrollVelocity.x *= 0.8f;
        }

        scrollOffsetTarget.x = CLAMP(scrollOffsetTarget.x, 0, maxScrollOffset.x);
        scrollOffset.x = LERP(scrollOffset.x, scrollOffsetTarget.x, 0.1);  // * dt ?
        scrollOffset.x = CLAMP(scrollOffset.x, 0, maxScrollOffset.x);
        if (maxScrollOffset.x - scrollOffset.x < 0.5f) {
            scrollOffset.x = maxScrollOffset.x;
        }

        const float scrollPct = scrollOffset.x / maxScrollOffset.x;
        const float scrollbarSize = MAX(scrollHandleThickness, panelRect.width * (panelRect.width / fullSize.x) - scrollHandleThickness);
        const float scrollbarSpace = panelRect.width - scrollHandleThickness - scrollbarSize;
        Rectangle scrollbarH{
            floorf(panelRect.x + scrollbarSpace * scrollPct),
            floorf(panelRect.y + panelRect.height - scrollHandleThickness),
            floorf(scrollbarSize),
            floorf(scrollHandleThickness)
        };
        DrawRectangleRec(scrollbarH, Fade(LIGHTGRAY, scrollHandleFade));
        DrawRectangleLinesEx(scrollbarH, 2, Fade(GRAY, scrollHandleFade));
    } else {
        scrollOffset.x = 0;
        scrollOffsetTarget.x = 0;
    }
    if (showScrollY) {
        if (scrollPanel.state.hover) {
            const float wheel = !io.KeyDown(KEY_LEFT_SHIFT) ? io.MouseWheelMove() : 0;
            if (wheel) {
                float impulse = 4 * fabsf(wheel);
                impulse *= impulse;
                if (wheel < 0) impulse *= -1;
                scrollVelocity.y += impulse;
                //printf("wheel: %f, target: %f\n", wheel, scrollOffsetTarget);
            }
        }

        if (scrollVelocity.y) {
            scrollOffsetTarget.y -= scrollVelocity.y;  // * dt
            scrollVelocity.y *= 0.8f;
        }

        scrollOffsetTarget.y = CLAMP(scrollOffsetTarget.y, 0, maxScrollOffset.y);
        scrollOffset.y = LERP(scrollOffset.y, scrollOffsetTarget.y, 0.1);  // * dt ?
        scrollOffset.y = CLAMP(scrollOffset.y, 0, maxScrollOffset.y);
        if (maxScrollOffset.y - scrollOffset.y < 0.5f) {
            scrollOffset.y = maxScrollOffset.y;
        }

        const float scrollYBottomMargin = scrollPanel.resizable ? scrollHandleThickness : 0;

        const float scrollPct = scrollOffset.y / maxScrollOffset.y;
        const float scrollbarSize = ceilf(MAX(scrollHandleThickness, panelRect.height * (panelRect.height / fullSize.y) - scrollYBottomMargin));
        const float scrollbarSpace = panelRect.height - scrollYBottomMargin - scrollbarSize;
        Rectangle scrollbarV{
            floorf(panelRect.x + panelRect.width - scrollHandleThickness),
            floorf(panelRect.y + scrollbarSpace * scrollPct),
            floorf(scrollHandleThickness),
            floorf(scrollbarSize)
        };
        DrawRectangleRec(scrollbarV, Fade(LIGHTGRAY, scrollHandleFade));
        DrawRectangleLinesEx(scrollbarV, 2, Fade(GRAY, scrollHandleFade));
    } else {
        scrollOffset.y = 0;
        scrollOffsetTarget.y = 0;
    }

#if 0
    const Vector2 mousePos = GetMousePosition();

    // Draw resize handle in bottom-right corner
    const int handleSize = 16;
    const int minSize = 80;

    Rectangle handleRect{
        scrollPanel.rect.x + scrollPanel.rect.width  - handleSize,
        scrollPanel.rect.y + scrollPanel.rect.height - handleSize,
        handleSize,
        handleSize
    };
    // TODO: Consider scissorRect?
    bool handleHovered = dlb_CheckCollisionPointRec(mousePos, handleRect);

    if (!dragPanel || dragPanel == &scrollPanel) {
        if (scrollPanel.uiState.hover) {
            io.CaptureMouse();
        }

        if (scrollPanel.uiState.hover) {
            if (io.MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (io.KeyDown(KEY_LEFT_SHIFT)) {
                    dragPanel = &scrollPanel;
                    dragPanelOffset = Vector2Subtract(mousePos, { scrollPanel.rect.x, scrollPanel.rect.y });
                    dragPanelMode = DragMode_Move;
                } else if (scrollPanel.resizable && handleHovered) {
                    const Vector2 panelBR{
                        scrollPanel.rect.x + scrollPanel.rect.width,
                        scrollPanel.rect.y + scrollPanel.rect.height
                    };

                    dragPanel = &scrollPanel;
                    dragPanelOffset = Vector2Subtract(panelBR, mousePos);
                    dragPanelMode = DragMode_Resize;
                }
            }
        }

        if (dragPanel == &scrollPanel) {
            if (io.MouseButtonDown(MOUSE_BUTTON_LEFT)) {
                switch (dragPanelMode) {
                    case DragMode_Move: {
                        Vector2 newPanelPos = Vector2Subtract(mousePos, dragPanelOffset);
                        scrollPanel.rect.x = newPanelPos.x;
                        scrollPanel.rect.y = newPanelPos.y;
                        break;
                    }
                    case DragMode_Resize: {
                        Vector2 mousePosRel = Vector2Subtract(mousePos, { scrollPanel.rect.x, scrollPanel.rect.y });
                        mousePosRel = Vector2Add(mousePosRel, dragPanelOffset);
                        scrollPanel.rect.width = MAX(minSize, mousePosRel.x);
                        scrollPanel.rect.height = MAX(minSize, mousePosRel.y);
                        break;
                    }
                }
            } else {
                dragPanel = 0;
                dragPanelOffset = {};
                dragPanelMode = DragMode_None;
            }
        }
    }

    if (scrollPanel.resizable) {
        Color handleColor = handleHovered ? SKYBLUE : GRAY;
        if (dragPanel == &scrollPanel && dragPanelMode == DragMode_Resize) {
            handleColor = ORANGE;
        }
        handleColor = ColorBrightness(handleColor, -0.5f);
        DrawRectangleRec(handleRect, Fade(handleColor, 0.5f));
        DrawRectangleLinesEx(handleRect, 2.0f, handleColor);
    }

    // Minimum panel size / distance from edge of screen
    scrollPanel.rect.x = CLAMP(scrollPanel.rect.x, -scrollPanel.rect.width + minSize, g_RenderSize.x - minSize);
    scrollPanel.rect.y = CLAMP(scrollPanel.rect.y, -scrollPanel.rect.height + minSize, g_RenderSize.y - minSize);
#endif

    cursor.x = scrollPanel.state.ctrlRect.x;
    cursor.y = scrollPanel.state.ctrlRect.y;
    UpdateCursor(scrollPanel.state.ctrlRect);
    io.PopScope();
}

UIState UI::Text(const char *text, size_t textLen)
{
    if (!textLen) {
        textLen = strlen(text);
        if (!textLen) {
            text = "<null>";
            textLen = strlen(text);
        }
    }

    const UIStyle &style = GetStyle();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 contentSize = style.size;
    if (contentSize.x <= 0 || contentSize.y <= 0) {
        Vector2 textSize = dlb_MeasureTextShadowEx(*style.font, text, textLen);
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
    UpdateCursor(ctrlRect);

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    Vector2 contentPos = {
        ctrlPosition.x + style.pad.left,
        ctrlPosition.y + style.pad.top
    };
    state.contentRect = { contentPos.x, contentPos.y, contentSize.x, contentSize.y };

    if (ShouldCull(ctrlRect)) return state;

    if (style.bgColor[UI_CtrlTypeDefault].a) {
        DrawRectangleRec(ctrlRect, style.bgColor[UI_CtrlTypeDefault]);
    }
    if (style.borderColor[UI_CtrlTypeDefault].a) {
        DrawRectangleLinesEx(ctrlRect, 1, style.borderColor[UI_CtrlTypeDefault]);
    }

    // Draw text
    dlb_DrawTextShadowEx(*style.font, text, textLen, contentPos, style.fgColor);

    return state;
}
UIState UI::Text(const std::string &text, Color fgColor, Color bgColor)
{
    UIStyle style = GetStyle();
    style.bgColor[UI_CtrlTypeDefault] = bgColor;
    if (fgColor.a) {
        style.fgColor = fgColor;
    }
    PushStyle(style);
    UIState state = Text(text.data(), text.size());
    PopStyle();
    return state;
}
UIState UI::Text(uint8_t value)
{
    const char *text = TextFormat("%" PRIu8, value);
    return Text(text);
}
UIState UI::Text(uint16_t value)
{
    const char *text = TextFormat("%" PRIu16, value);
    return Text(text);
}
UIState UI::Text(uint32_t value)
{
    const char *text = TextFormat("%" PRIu32, value);
    return Text(text);
}
UIState UI::Text(uint64_t value)
{
    const char *text = TextFormat("%" PRIu64, value);
    return Text(text);
}
template <typename T>
UIState UI::Text(T value)
{
    const char *text = TextFormat("<%s>", typeid(T).name());
    PushFgColor(Fade(LIGHTGRAY, 0.5f));
    UIState result = Text(text);
    PopStyle();
    return result;
}
UIState UI::Label(const std::string &text, int width)
{
    if (width) PushWidth(width);
    UIState state = Text(text);
    if (width) PopStyle();
    return state;
}

UIState UI::Image(const Texture &texture, Rectangle srcRect, bool checkerboard)
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
        ctrlSize.x + style.borderWidth[UI_CtrlTypeDefault] * 2,
        ctrlSize.y + style.borderWidth[UI_CtrlTypeDefault] * 2
    };
    UpdateCursor(ctrlRect);

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    Rectangle contentRect = ctrlRect;
    contentRect.x += style.borderWidth[UI_CtrlTypeDefault];
    contentRect.y += style.borderWidth[UI_CtrlTypeDefault];
    contentRect.width -= style.borderWidth[UI_CtrlTypeDefault] * 2;
    contentRect.height -= style.borderWidth[UI_CtrlTypeDefault] * 2;
    state.contentRect = contentRect;

    if (ShouldCull(ctrlRect)) return state;

    // Draw border
    DrawRectangleLinesEx(ctrlRect, style.borderWidth[UI_CtrlTypeDefault], style.borderColor[UI_CtrlTypeDefault]);

    // Draw checkerboard background
    if (checkerboard) {
        DrawTexturePro(checkerTex, srcRect, contentRect, {}, 0, WHITE);
    }

    // Draw image
    DrawTexturePro(texture, srcRect, contentRect, {}, 0, WHITE);

    UpdateAudio(state);
    return state;
}
UIState UI::Checkerboard(Rectangle rect)
{
    return Image(checkerTex, rect, false);
}

UIState UI::Button(const std::string &text)
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
        Vector2 textSize = dlb_MeasureTextShadowEx(*style.font, text.data(), text.size());
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
        ctrlSize.x + style.borderWidth[UI_CtrlTypeButton] * 2,
        ctrlSize.y + style.borderWidth[UI_CtrlTypeButton] * 2
    };
    UpdateCursor(ctrlRect);

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    const float downOffset = (style.buttonPressed || state.down) ? 1 : -1;
    Rectangle contentRect{
        ctrlPosition.x + style.borderWidth[UI_CtrlTypeButton],
        ctrlPosition.y + style.borderWidth[UI_CtrlTypeButton] * downOffset,
        ctrlSize.x,
        ctrlSize.y
    };
    state.contentRect = contentRect;

    if (ShouldCull(ctrlRect)) return state;

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

    // Draw button
    DrawRectangleRounded(contentRect, cornerRoundness, cornerSegments, bgColorFx);

    // Draw button text
    dlb_DrawTextShadowEx(*style.font, text.data(), text.size(),
        {
            contentRect.x + style.pad.left,
            contentRect.y + style.pad.top
        },
        fgColorFx
    );

    UpdateAudio(state);
    return state;
}
UIState UI::Button(const std::string &text, Color bgColor)
{
    UIStyle style = GetStyle();
    style.bgColor[UI_CtrlTypeButton] = bgColor;
    PushStyle(style);
    UIState state = Button(text);
    PopStyle();
    return state;
}
UIState UI::ToggleButton(const std::string &text, bool pressed, Color bgColor, Color bgColorPressed)
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

size_t RN_linelen(const char *str)
{
    if (!str) return 0;

    size_t len = 0;
    const char *c = str;
    while (*c && *c != '\n') c++;
    if (*c) c++;
    len = c - str;
    return len;
}

void RN_stb_layout_row(StbTexteditRow *row, StbString *str, int startIndex)
{
    const char *line = str->data.c_str() + startIndex;
    size_t linelen = RN_linelen(line);
    size_t textlen = linelen;

    if (line[linelen - 1] == '\n') {
        textlen--;
    }
    Vector2 textSize = dlb_MeasureTextShadowEx(*str->font, line, textlen);

    if (linelen == 1 && line[0] == '\n') {
        textSize.y = (float)(str->font->baseSize / 2 - 1);  // newlines are half space, no clue why we need -1 though... -_-
    }

    row->x0 = 0;
    row->x1 = textSize.x;
    row->baseline_y_delta = textSize.y;
    row->ymin = 0;
    row->ymax = textSize.y;
    row->num_chars = linelen;
}
int RN_stb_get_char_width(StbString *str, int startIndex, int offset) {
    std::string oneChar = str->data.substr((size_t)startIndex + offset, 1);
    Vector2 charSize = dlb_MeasureTextShadowEx(*str->font, CSTRS(oneChar));
    return charSize.x;
}
int RN_stb_key_to_char(int key)
{
    if (isascii(key)) {
        return key;
    } else if (key == KEY_ENTER || key == KEY_KP_ENTER) {
        return '\n';
    } else {
        return -1;
    }
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

UIState UI::TextboxWithDefault(uint32_t ctrlid, std::string &text, const std::string &placeholder, bool multiline, KeyPreCallback preCallback, KeyPostCallback postCallback, void *userData)
{
    assert(ctrlid);  // if 0, tabbing will break. unfortunately __COUNTER__ starts at 0. :(

    static STB_TexteditState stbState{};

    const UIStyle &style = GetStyle();
    StbString str{ style.font, text };

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 contentSize = style.size;
    if (contentSize.x <= 0 || contentSize.y <= 0) {
        const std::string &tmp = text.size() ? text : " ";
        Vector2 textSize = dlb_MeasureTextShadowEx(*style.font, CSTRS(tmp));
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

    //--------------------------------------------------------------------------
    // Focus
    //--------------------------------------------------------------------------
    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);
    if (state.pressed || tabToNextEditor || tabToPrevEditor == ctrlid) {
        prevActiveEditor = activeEditor;
        activeEditor = ctrlid;
        tabToNextEditor = false;
        tabToPrevEditor = 0;
    }

    const bool wasActive = prevActiveEditor == ctrlid;
    const bool isActive = activeEditor == ctrlid;
    const bool newlyActive = isActive && (/*!stbState.initialized || */!wasActive);

    if (isActive) {
        //--------------------------------------------------------------------------
        // Mouse
        //--------------------------------------------------------------------------
        Vector2 mousePosRel{
            GetMouseX() - (ctrlPosition.x + style.pad.left),
            GetMouseY() - (ctrlPosition.y + style.pad.top)
        };

        static double lastClickTime{};
        static Vector2 lastClickMousePosRel{};
        static int mouseClicks{};
        static int dblClickWordLeft{};
        static int dblClickWordRight{};
        static bool newlyActivePress{};

        const double now = GetTime();
        const double timeSinceLastClick = now - lastClickTime;

        if (newlyActive) {
            stb_textedit_initialize_state(&stbState, !multiline);
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

        //--------------------------------------------------------------------------
        // Keyboard
        //--------------------------------------------------------------------------
        bool keyHandled = false;
        if (!io.KeyboardCaptured() && preCallback) {
            preCallback(str.data, userData, keyHandled);
        }

        if (!io.KeyboardCaptured() && !keyHandled) {
            bool ctrl = io.KeyDown(KEY_LEFT_CONTROL) || io.KeyDown(KEY_RIGHT_CONTROL);
            bool shift = io.KeyDown(KEY_LEFT_SHIFT) || io.KeyDown(KEY_RIGHT_SHIFT);

            int mod = 0;
            if (ctrl) mod |= STB_TEXTEDIT_K_CTRL;
            if (shift) mod |= STB_TEXTEDIT_K_SHIFT;

#define STB_KEY(key) if (io.KeyPressed(key, true)) { stb_textedit_key(&str, &stbState, mod | key); }
            STB_KEY(KEY_ENTER);
            STB_KEY(KEY_KP_ENTER);
            STB_KEY(KEY_LEFT);
            STB_KEY(KEY_RIGHT);
            STB_KEY(KEY_UP);
            STB_KEY(KEY_DOWN);
            STB_KEY(KEY_DELETE);
            STB_KEY(KEY_BACKSPACE);
            STB_KEY(KEY_HOME);
            STB_KEY(KEY_END);
            STB_KEY(KEY_PAGE_UP);
            STB_KEY(KEY_PAGE_DOWN);
#undef STB_KEY

            if (ctrl && io.KeyPressed(KEY_A)) {
                stbState.cursor = text.size();
                stbState.select_start = 0;
                stbState.select_end = text.size();
            }
            if (ctrl && io.KeyPressed(KEY_X, true)) {
                int selectLeft = MIN(stbState.select_start, stbState.select_end);
                int selectRight = MAX(stbState.select_start, stbState.select_end);
                int selectLen = selectRight - selectLeft;
                if (selectLen) {
                    std::string selectedText = str.data.substr(selectLeft, selectLen);
                    SetClipboardText(selectedText.c_str());
                    stb_textedit_cut(&str, &stbState);
                }
            }
            if (ctrl && io.KeyPressed(KEY_C, false)) {
                int selectLeft = MIN(stbState.select_start, stbState.select_end);
                int selectRight = MAX(stbState.select_start, stbState.select_end);
                int selectLen = selectRight - selectLeft;
                if (selectLen) {
                    std::string selectedText = str.data.substr(selectLeft, selectLen);
                    SetClipboardText(selectedText.c_str());
                }
            }
            if (ctrl && io.KeyPressed(KEY_V, true)) {
                const char *clipboard = GetClipboardText();
                if (clipboard) {
                    size_t clipboardLen = strlen(clipboard);
                    if (clipboardLen) {
                        stb_textedit_paste(&str, &stbState, clipboard, clipboardLen);
                    }
                }
            }
            if (io.KeyPressed(KEY_TAB, true) && !tabHandledThisFrame) {
                if (shift) {
                    // tell previously drawn textbox to activate next frame
                    tabToPrevEditor = lastDrawnEditor;
                } else {
                    // tell next textbox that draws to activate
                    tabToNextEditor = true;
                }
                tabHandledThisFrame = true;
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

                if (RN_stb_key_to_char(ch) != -1) {
                    stb_textedit_key(&str, &stbState, ch);
                }
                ch = GetCharPressed();
            }
        }

        if (!io.KeyboardCaptured() && postCallback) {
            postCallback(str.data, userData);
        }
    }

    //--------------------------------------------------------------------------
    // Draw
    //--------------------------------------------------------------------------
    // Background
    const Color bgColor = style.bgColor[UI_CtrlTypeTextbox];
    if (!bgColor.a) {
        // TODO(dlb)[cleanup]: What da heck is dis??
        //bgColor = DARKGRAY;
    }
    if (bgColor.a) {
        DrawRectangleRec(ctrlRect, bgColor);
    }

    // Border
    Color borderColor = style.borderColor[UI_CtrlTypeTextbox];
    if (borderColor.a) {
        DrawRectangleLinesEx(ctrlRect, 1, borderColor);
    }

    // Text
    if (text.size() || isActive) {
        dlb_DrawTextShadowEx(*style.font, text.c_str(), text.size(),
            { ctrlPosition.x + style.pad.left, ctrlPosition.y + style.pad.top }, RAYWHITE);
    } else {
        dlb_DrawTextShadowEx(*style.font, placeholder.c_str(), placeholder.size(),
            { ctrlPosition.x + style.pad.left, ctrlPosition.y + style.pad.top }, Fade(RAYWHITE, 0.6f));
    }

    // Selection highlight
    if (isActive) {
        if (stbState.select_start != stbState.select_end) {
            int selectLeft = MIN(stbState.select_start, stbState.select_end);
            int selectRight = MAX(stbState.select_start, stbState.select_end);

            int startIndex = selectLeft;
            Vector2 cursor{};

            // Measure text before selection to find beginning offset
            if (startIndex) {
                // NOTE(dlb)[perf]: For strings with many lines, we should findRowStart(str)
                // and measure from there instead.
                dlb_MeasureTextShadowEx(*style.font, text.c_str(), startIndex, &cursor);
            }

            // Draw rects for each row within the selection
            StbTexteditRow row{};
            while (true) {
                RN_stb_layout_row(&row, &str, startIndex);

                float width = (row.x1 - row.x0);
                float height = (row.ymax - row.ymin);

                if (startIndex + row.num_chars > selectRight) {
                    Vector2 lastLineSize = dlb_MeasureTextShadowEx(*style.font, text.c_str() + startIndex, selectRight - startIndex, 0);
                    width = lastLineSize.x;
                }

                Rectangle rowRect{
                    ctrlRect.x + style.pad.left + cursor.x,
                    ctrlRect.y + style.pad.top + cursor.y,
                    width,
                    height
                };

                DrawRectangleRec(rowRect, Fade(SKYBLUE, 0.5f));

                startIndex += row.num_chars;
                if (startIndex >= selectRight) {
                    break;
                }

                cursor.x = 0;
                cursor.y += row.baseline_y_delta;
            }
        }

        Vector2 measureCursor{};
        if (stbState.cursor) {
            std::string textBeforeCursor = text.substr(0, stbState.cursor);
            dlb_MeasureTextShadowEx(*style.font, CSTRS(textBeforeCursor), &measureCursor);
        }
        Vector2 cursorPos = Vector2Add(ctrlPosition, Vector2Add({ style.pad.left, style.pad.top }, measureCursor));
        Rectangle cursorRect{
            cursorPos.x,
            cursorPos.y,
            1,
            (float)style.font->baseSize
        };
        DrawRectangleRec(cursorRect, RAYWHITE);
    }

    state.contentRect = ctrlRect;
    UpdateCursor(ctrlRect);

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

    lastDrawnEditor = ctrlid;
    return state;
}

UIState UI::Textbox(uint32_t ctrlid, std::string &text, bool multiline, KeyPreCallback preCallback, KeyPostCallback postCallback, void *userData)
{
    return TextboxWithDefault(ctrlid, text, "", multiline, preCallback, postCallback, userData);
}

struct TextboxFloatCallbackData {
    const char *fmt;
    float *val;
    float increment;
};

void AdjustFloat(std::string &str, void *userData, bool &keyHandled)
{
    TextboxFloatCallbackData *data = (TextboxFloatCallbackData *)userData;

    if (io.KeyPressed(KEY_UP, true)) {
        *data->val += data->increment;
        str = TextFormat(data->fmt, *data->val);
        keyHandled = true;
    }
    if (io.KeyPressed(KEY_DOWN, true)) {
        *data->val -= data->increment;
        str = TextFormat(data->fmt, *data->val);
        keyHandled = true;
    }
}

UIState UI::Textbox(uint32_t ctrlid, float &value, const char *fmt, float increment)
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
    TextboxFloatCallbackData data{ fmt, &value, increment };
    UIState state = Textbox(ctrlid, valueStr, true, AdjustFloat, 0, (void *)&data);
    char *end = 0;
    float newValue = strtof(valueStr.c_str(), &end);
    if (*end != '\0') {
        // todo: check errors before saving float value
    }
    value = newValue;
    return state;
#endif
}

void LimitStringLength(std::string &str, void *userData)
{
    size_t maxLength = (size_t)userData;
    if (str.size() > maxLength) {
        str.resize(maxLength, 0);
    }
}

template <typename T>
void UI::HAQFieldValue(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth)
{
    PushFgColor(RED);
    Label("No UI Renderer");
    PopStyle();
}

#define HAQ_UI_FIELD(c_type, c_name, c_init, flags, condition, userdata) \
    if (condition) { \
        size_t hash = hash_combine(__COUNTER__, ctrlid, name, #c_name); \
        HAQField(hash, #c_name, userdata.c_name, (flags), labelWidth); \
    }

#define HAQ_UI_DAT(c_type, hqt) \
    void UI::HAQFieldValue(uint32_t ctrlid, const std::string &name, c_type &dat, int flags, int labelWidth) \
    { \
        hqt(HAQ_UI_FIELD, dat); \
    }

HAQ_UI_DAT(GfxFile   , HQT_GFX_FILE_FIELDS);
HAQ_UI_DAT(MusFile   , HQT_MUS_FILE_FIELDS);
HAQ_UI_DAT(SfxFile   , HQT_SFX_FILE_FIELDS);
HAQ_UI_DAT(GfxFrame  , HQT_GFX_FRAME_FIELDS);
HAQ_UI_DAT(GfxAnim   , HQT_GFX_ANIM_FIELDS);
HAQ_UI_DAT(ObjectData, HQT_OBJECT_DATA_FIELDS);
HAQ_UI_DAT(Sprite    , HQT_SPRITE_FIELDS);
HAQ_UI_DAT(TileDef   , HQT_TILE_DEF_FIELDS);
HAQ_UI_DAT(TileMat   , HQT_TILE_MAT_FIELDS);
HAQ_UI_DAT(Tilemap   , HQT_TILE_MAP_FIELDS);

#undef HAQ_UI_DAT
#undef HAQ_UI_FIELD

template <typename T>
void UI::HAQFieldValueArray(uint32_t ctrlid, const std::string &name, T *data, size_t count, int flags, int labelWidth)
{
    PushIndent();
    Newline();

    for (int i = 0; i < count; i++) {
        std::string name_i = TextFormat("%s[%d]", name.c_str(), i);

        PushFgColor(GRAY);
        Label(name_i, labelWidth);
        PopStyle();
        Newline();

        size_t hash = hash_combine(__COUNTER__, ctrlid, name, i);
        HAQField(hash, "#" + name_i, data[i], flags, labelWidth);
    }

    PopStyle();
}

template <typename T, size_t S>
void UI::HAQFieldValue(uint32_t ctrlid, const std::string &name, std::array<T, S> &arr, int flags, int labelWidth)
{
    HAQFieldValueArray(ctrlid, name, arr.data(), arr.size(), flags, labelWidth);
}

template <typename T>
void UI::HAQFieldValue(uint32_t ctrlid, const std::string &name, std::vector<T> &vec, int flags, int labelWidth)
{
    HAQFieldValueArray(ctrlid, name, vec.data(), vec.size(), flags, labelWidth);
}

template <typename T>
void UI::HAQFieldEditor(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth)
{
    int popStyle = 0;

    const UIStyle &style = GetStyle();
    const float textboxWidth = 400 - 24 - labelWidth - style.indent * style.font->baseSize;
    PushWidth(textboxWidth);
    popStyle++;

    if (flags & HAQ_STYLE_BGCOLOR_RED) {
        PushBgColor({ 127, 0, 0, 255 }, UI_CtrlTypeTextbox);
        popStyle++;
    } else if (flags & HAQ_STYLE_BGCOLOR_GREEN) {
        PushBgColor({ 0, 127, 0, 255 }, UI_CtrlTypeTextbox);
        popStyle++;
    } else if (flags & HAQ_STYLE_BGCOLOR_BLUE) {
        PushBgColor({ 0, 0, 127, 255 }, UI_CtrlTypeTextbox);
        popStyle++;
    }

    if constexpr (std::is_same_v<T, std::string>) {
        Textbox(ctrlid, value, flags & HAQ_STYLE_STRING_MULTILINE);
    } else if constexpr (std::is_same_v<T, float>) {
        const char* floatFmt = "%f";
        float floatInc = 1.0f;
        if (flags & HAQ_STYLE_FLOAT_TENTH) {
            floatFmt = "%.1f";
            floatInc = 0.1f;
        } else if (flags & HAQ_STYLE_FLOAT_HUNDRETH) {
            floatFmt = "%.2f";
            floatInc = 0.01f;
        }
        Textbox(ctrlid, value, floatFmt, floatInc);
    } else if constexpr (
        std::is_same_v<T, uint8_t > ||
        std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, int8_t  > ||
        std::is_same_v<T, int16_t > ||
        std::is_same_v<T, int32_t >
        ) {
        float valueFloat = (float)value;
        UIState state = Textbox(ctrlid, valueFloat, "%.f");
        if (flags & HAQ_HOVER_SHOW_TILE && state.hover) {
            DrawTile(value, GetMousePosition(), &tooltips);
        }
        value = CLAMP(valueFloat, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    } else {
        HAQFieldValue(ctrlid, name, value, flags, labelWidth);
    }

    while (popStyle--) PopStyle();
}

void UI::HAQFieldEditor(uint32_t ctrlid, const std::string &name, TileDef::Flags &value, int flags, int labelWidth)
{
    PushWidth(60);

    uint8_t bitflags = value;
    if (ToggleButton("Solid", bitflags & TileDef::FLAG_SOLID, GRAY, SKYBLUE).pressed) {
        bitflags ^= TileDef::FLAG_SOLID;
    }
    if (ToggleButton("Liquid", bitflags & TileDef::FLAG_LIQUID, GRAY, SKYBLUE).pressed) {
        bitflags ^= TileDef::FLAG_LIQUID;
    }
    value = (TileDef::Flags)bitflags;

    PopStyle();
}

void UI::HAQFieldEditor(uint32_t ctrlid, const std::string &name, ObjType &value, int flags, int labelWidth)
{
    uint32_t vint = value;
    const float typeStrWidth = 80;

    PushWidth(typeStrWidth);
    UIState typeText = Text(ObjTypeStr(value));
    PopStyle();

    const UIStyle &style = GetStyle();
    const float pad = style.pad.left + style.pad.right;
    const float margin = style.margin.left + style.margin.right;
    const float typeStrSpace = typeStrWidth + pad + margin;
    HAQFieldEditor(ctrlid, "", vint, flags, labelWidth + typeStrSpace);

    value = (ObjType)CLAMP(vint, 1, OBJ_COUNT - 1);
}

template <typename T>
void UI::HAQField(uint32_t ctrlid, const std::string &name, T &value, int flags, int labelWidth)
{
    if (name.size() && name[0] != '#') {
        if (name.size()) {
            Label(name, labelWidth);
        } else {
            Space({ (float)labelWidth, 0 });
        }
    }

    if ((flags) & HAQ_EDIT) {
        HAQFieldEditor(ctrlid, name, value, flags, labelWidth);
    } else {
        Text(value);
    }

    Newline();
}

void UI::Tooltip(const std::string &text, Vector2 position)
{
    tooltips.push(DrawCmd::Tooltip(text, position));
}

void UI::Tooltip(const std::string &text)
{
    Tooltip(text, Vector2Add(GetMousePosition(), { 16, 16 }));
}

void UI::DrawTooltips(void)
{
    tooltips.Draw();
}