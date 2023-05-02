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

UIState UI::Button(const char *text)
{
    UIStyle &style = styleStack.top();

    Vector2 buttonPos{
        position.x + cursor.x + style.margin.left,
        position.y + cursor.y + style.margin.top
    };
    const float cornerRoundness = 0.2f;
    const float cornerSegments = 4;

    Vector2 textSize = MeasureTextEx(*style.font, text, style.font->baseSize, 1.0f);
    Vector2 buttonSize{
        style.pad.left + textSize.x + style.pad.right,
        style.pad.top + textSize.y + style.pad.bottom
    };

    Rectangle buttonRect = {
        buttonPos.x,
        buttonPos.y,
        buttonSize.x + style.borderThickness.x * 2,
        buttonSize.y + style.borderThickness.y * 2
    };

    // Draw drop shadow
    DrawRectangleRounded(buttonRect, cornerRoundness, cornerSegments, BLACK);

    static struct HoverHash {
        const char *text{};
        Vector2 position{};

        HoverHash(void) {};
        HoverHash(const char *text, Vector2 position) : text(text), position(position) {}

        bool Equals(HoverHash &other) {
            return text == other.text && Vector2Equals(position, other.position);
        }
    } prevHoverHash{};

    HoverHash hash{ text, { buttonRect.x, buttonRect.y } };

    Color effectiveColor = style.bgColor;
    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), buttonRect)) {
        state.hover = true;
        effectiveColor = ColorBrightness(style.bgColor, 0.3f);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            effectiveColor = ColorBrightness(style.bgColor, -0.3f);
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
            buttonPos.x + style.borderThickness.x,
            buttonPos.y + style.borderThickness.y * downOffset,
            buttonSize.x,
            buttonSize.y
        },
        cornerRoundness, cornerSegments, effectiveColor
    );

    // Draw button text
    DrawTextShadowEx(*style.font, text,
        {
            buttonPos.x + style.borderThickness.x + style.pad.left,
            buttonPos.y + style.borderThickness.y * downOffset + style.pad.top
        },
        style.fgColor
    );

    // How much total space we used up (including margin)
    Vector2 buttonSpace{
        style.margin.left + buttonRect.width + style.margin.right,
        style.margin.top + buttonRect.height + style.margin.bottom,
    };
    lineSize.x += buttonSpace.x;
    lineSize.y = MAX(buttonSpace.y, lineSize.y);
    cursor.x += buttonSpace.x;

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