#include "ui.h"
#include "../shared_lib.h"

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

UIStyle UI::GetStyle(void)
{
    UIStyle style = styleStack.top();
    return style;
}

void UI::PopStyle(void)
{
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
            state.clicked = true;
        }
        prevHoverHash = hash;
    } else if (prevHoveredCtrl) {
        prevHoverHash = {};
    }
    return state;
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
    UIStyle &style = styleStack.top();

    Vector2 ctrlPosition{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };

    Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
    Vector2 ctrlSize{
        style.pad.left + textSize.x + style.pad.right,
        style.pad.top + textSize.y + style.pad.bottom
    };

    Align(style, ctrlPosition, ctrlSize);

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.borderThickness.x * 2,
        ctrlSize.y + style.borderThickness.y * 2
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    // Draw text
    DrawTextShadowEx(*style.font, text,
        {
            ctrlPosition.x + style.borderThickness.x + style.pad.left,
            ctrlPosition.y + style.borderThickness.y * style.pad.top
        },
        style.fgColor
    );

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

    UIStyle &style = styleStack.top();

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
        ctrlSize.x,
        ctrlSize.y
    };

    static HoverHash prevHoverHash{};
    UIState state = CalcState(ctrlRect, prevHoverHash);

    // Draw image
    DrawTexturePro(texture, srcRect, ctrlRect, {}, 0, WHITE);

    if (state.hover) {
        DrawRectangleLinesEx(ctrlRect, 2, YELLOW);
    }

    // Audio
    if (state.clicked) {
        if (!IsSoundPlaying(sndHardTick)) PlaySound(sndHardTick);
    } else if (state.entered) {
        if (!IsSoundPlaying(sndSoftTick)) PlaySound(sndSoftTick);
    }

    UpdateCursor(style, ctrlRect);
    return state;
}

UIState UI::Button(const char *text)
{
    UIStyle &style = styleStack.top();

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

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.borderThickness.x * 2,
        ctrlSize.y + style.borderThickness.y * 2
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

    // Draw button
    DrawRectangleRounded(
        {
            ctrlPosition.x + style.borderThickness.x,
            ctrlPosition.y + style.borderThickness.y * downOffset,
            ctrlSize.x,
            ctrlSize.y
        },
        cornerRoundness, cornerSegments, bgColorFx
    );

    // Draw button text
    DrawTextShadowEx(*style.font, text,
        {
            ctrlPosition.x + style.borderThickness.x + style.pad.left,
            ctrlPosition.y + style.borderThickness.y * downOffset + style.pad.top
        },
        fgColorFx
    );

    // Audio
    if (state.clicked) {
        PlaySound(sndHardTick);
    } else if (state.entered) {
        PlaySound(sndSoftTick);
    }

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

UIState UI::Button(const char *text, bool pressed, Color pressedColor)
{
    UIState state{};
    if (pressed) {
        UIStyle style = GetStyle();
        style.bgColor = pressedColor;
        style.buttonPressed = pressed;
        PushStyle(style);
    }
    state = Button(text);
    if (pressed) {
        PopStyle();
    }
    return state;
}