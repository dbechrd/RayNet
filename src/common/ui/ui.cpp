#include "ui.h"
#include "../shared_lib.h"

Sound sndSoftTick;
Sound sndHardTick;

Err InitUI(void)
{
    Err err = RN_SUCCESS;

    sndSoftTick = LoadSound("resources/soft_tick.wav");
    if (!sndSoftTick.frameCount) err = RN_RAYLIB_ERROR;
    sndHardTick = LoadSound("resources/hard_tick.wav");
    if (!sndHardTick.frameCount) err = RN_RAYLIB_ERROR;

    return err;
}

void FreeUI(void)
{
    UnloadSound(sndSoftTick);
    UnloadSound(sndHardTick);
}

UIState UIButton(Font font, Color color, const char *text, Vector2 uiPosition, Vector2 &uiCursor)
{
    Vector2 position = Vector2Add(uiPosition, uiCursor);
    const Vector2 pad{ 8, 1 };
    const float cornerRoundness = 0.2f;
    const float cornerSegments = 4;
    const Vector2 lineThick{ 1.0f, 1.0f };

    Vector2 textSize = MeasureTextEx(font, text, font.baseSize, 1.0f);
    Vector2 buttonSize = textSize;
    buttonSize.x += pad.x * 2;
    buttonSize.y += pad.y * 2;

    Rectangle buttonRect = {
        position.x - lineThick.x,
        position.y - lineThick.y,
        buttonSize.x + lineThick.x * 2,
        buttonSize.y + lineThick.y * 2
    };
    if (lineThick.x || lineThick.y) {
        DrawRectangleRounded(
            buttonRect,
            cornerRoundness, cornerSegments, BLACK
        );
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

    Color effectiveColor = color;
    UIState state{};
    if (dlb_CheckCollisionPointRec(GetMousePosition(), buttonRect)) {
        state.hover = true;
        effectiveColor = ColorBrightness(color, 0.3f);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.down = true;
            effectiveColor = ColorBrightness(color, -0.3f);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            state.clicked = true;
        }
    }

    if (state.clicked) {
        PlaySound(sndHardTick);
    } else if (state.hover) {
        HoverHash hash{ text, { buttonRect.x, buttonRect.y } };
        if (!hash.Equals(prevHoverHash)) {
            PlaySound(sndSoftTick);
            prevHoverHash = hash;
        }
    }

    float yOffset = (state.down ? 0 : -lineThick.y * 2);
    DrawRectangleRounded(
        { position.x, position.y + yOffset, buttonSize.x, buttonSize.y },
        cornerRoundness, cornerSegments, effectiveColor);
    DrawTextShadowEx(font, text,
        { position.x + pad.x, position.y + pad.y + yOffset },
        font.baseSize, WHITE);

    uiCursor.x += buttonSize.x;
    return state;
}