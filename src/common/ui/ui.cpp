#include "ui.h"
#include "../shared_lib.h"

UI::UI(Vector2 position, UIStyle style)
{
    this->position = position;
    styleStack.push(style);
}

void UI::Newline(void)
{
    cursor.x = 0;
    cursor.y += lineSize.y;
    lineSize = {};
}

void UI::PushStyle(UIStyle style)
{
    styleStack.push(style);
}

void UI::PopStyle(void)
{
    styleStack.pop();
}

void UI::Text(const char *text)
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

    switch (style.alignH) {
        case TextAlign_Left: {
            break;
        }
        case TextAlign_Center: {
            ctrlPosition.x -= ctrlSize.x / 2;
            ctrlPosition.y -= ctrlSize.y / 2;
            break;
        }
        case TextAlign_Right: {
            ctrlPosition.x -= ctrlSize.x;
            ctrlPosition.y -= ctrlSize.y;
            break;
        }
    }

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.borderThickness.x * 2,
        ctrlSize.y + style.borderThickness.y * 2
    };

    // Draw text
    DrawTextShadowEx(*style.font, text,
        {
            ctrlPosition.x + style.borderThickness.x + style.pad.left,
            ctrlPosition.y + style.borderThickness.y * style.pad.top
        },
        style.fgColor
    );

    // How much total space we used up (including margin)
    Vector2 ctrlSpaceUsed{
        style.margin.left + ctrlRect.width + style.margin.right,
        style.margin.top + ctrlRect.height + style.margin.bottom,
    };
    lineSize.x += ctrlSpaceUsed.x;
    lineSize.y = MAX(ctrlSpaceUsed.y, lineSize.y);
    cursor.x += ctrlSpaceUsed.x;
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

    switch (style.alignH) {
        case TextAlign_Left: {
            break;
        }
        case TextAlign_Center: {
            ctrlPosition.x -= ctrlSize.x / 2;
            ctrlPosition.y -= ctrlSize.y / 2;
            break;
        }
        case TextAlign_Right: {
            ctrlPosition.x -= ctrlSize.x;
            ctrlPosition.y -= ctrlSize.y;
            break;
        }
    }

    Rectangle ctrlRect = {
        ctrlPosition.x,
        ctrlPosition.y,
        ctrlSize.x + style.borderThickness.x * 2,
        ctrlSize.y + style.borderThickness.y * 2
    };

    // Draw drop shadow
    if (style.bgColor.a) {
        DrawRectangleRounded(ctrlRect, cornerRoundness, cornerSegments, BLACK);
    }

    static struct HoverHash {
        const char *text{};
        Vector2 position{};

        HoverHash(void) {};
        HoverHash(const char *text, Vector2 position) : text(text), position(position) {}

        bool Equals(HoverHash &other) {
            return text == other.text && Vector2Equals(position, other.position);
        }
    } prevHoverHash{};

    HoverHash hash{ text, { ctrlRect.x, ctrlRect.y } };

    Color bgColorFx = style.bgColor;
    Color fgColorFx = style.fgColor;
    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), ctrlRect)) {
        state.hover = true;
        bgColorFx = ColorBrightness(style.bgColor, 0.3f);
        fgColorFx = YELLOW;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            bgColorFx = ColorBrightness(style.bgColor, -0.3f);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                state.pressed = true;
            }
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state.clicked = true;
        }
    } else if (hash.Equals(prevHoverHash)) {
        prevHoverHash = {};
    }

    if (state.clicked) {
        PlaySound(sndHardTick);
    } else if (state.hover) {
        if (!hash.Equals(prevHoverHash)) {
            PlaySound(sndSoftTick);
            prevHoverHash = hash;
        }
    }

    const float downOffset = (state.down ? 1 : -1);

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

    // How much total space we used up (including margin)
    Vector2 ctrlSpaceUsed{
        style.margin.left + ctrlRect.width + style.margin.right,
        style.margin.top + ctrlRect.height + style.margin.bottom,
    };
    lineSize.x += ctrlSpaceUsed.x;
    lineSize.y = MAX(ctrlSpaceUsed.y, lineSize.y);
    cursor.x += ctrlSpaceUsed.x;

    return state;
}

UIState UI::Button(const char *text, Color bgColor)
{
    UIStyle colorStyle = styleStack.top();
    colorStyle.bgColor = bgColor;
    PushStyle(colorStyle);
    UIState state = Button(text);
    PopStyle();
    return state;
}