#include "histogram.h"
#include "collision.h"

const float Histogram::barPadding = 1.0f;
const float Histogram::barWidth = 7.0f;
const float Histogram::histoHeight = 40.0f;
bool Histogram::paused = false;

Histogram *Histogram::hoveredHisto;
int Histogram::hoveredIdx;

Histogram histoFps;
Histogram histoInput;
Histogram histoDx;

void Histogram::Push(Entry &entry)
{
    if (!paused) {
        buffer.push(entry);
    }
}

void Histogram::ResetHover(void)
{
    hoveredHisto = 0;
    hoveredIdx = -1;
}

void Histogram::Draw(Vector2 position)
{
    float maxValue = 0.0f;
    for (int i = 0; i < buffer.size(); i++) {
        maxValue = MAX(maxValue, buffer[i].value);
    }

    const float barScale = histoHeight / maxValue;

    Vector2 uiCursor{ position };

    const Vector2 mousePos = GetMousePosition();

    for (int i = 0; i < buffer.size(); i++) {
        float bottom = uiCursor.y + histoHeight;
        float height = buffer[i].value * barScale;

        Rectangle rect{};
        rect.x = uiCursor.x;
        rect.y = bottom - height;
        rect.width = barWidth;
        rect.height = height;
        DrawRectangleRec(rect, buffer[i].color.a ? buffer[i].color : RAYWHITE);

        Rectangle hover = rect;
        hover.y = position.y;
        hover.height = histoHeight;
        // Including padding between rects in collision to prevent tooltip from
        // disappearing when exactly on a boundary causing flickering.
        hover.x -= barPadding;
        hover.width += barPadding;
        if (dlb_CheckCollisionPointRec(mousePos, hover)) {
            DrawRectangleLinesEx(hover, 1, ORANGE);
            hoveredHisto = this;
            hoveredIdx = i;
        }

        uiCursor.x += barWidth + barPadding;
    }
}

void Histogram::DrawHover(void)
{
    if (hoveredHisto && hoveredIdx >= 0) {
        const Vector2 mousePos = GetMousePosition();

        Entry &entry = hoveredHisto->buffer[hoveredIdx];
        const float tipPad = 8;
        Vector2 tipPos = Vector2Add(mousePos, { 12 + tipPad, 12 + tipPad });
        const char *tipStr = TextFormat(
            "frame %u\n"
            "now   %.3f\n"
            "value %.3f%s"
            "%s",
            entry.frame,
            entry.now,
            entry.value, entry.metadata.size() ? "\n" : "",
            entry.metadata.c_str()
        );
        Vector2 tipSize = MeasureTextEx(fntHackBold20, tipStr, fntHackBold20.baseSize, 1);
        Rectangle tipRect{ tipPos.x, tipPos.y, tipSize.x, tipSize.y };
        tipRect = RectGrow(tipRect, tipPad);
        tipRect = RectConstrainToScreen(tipRect);
        tipPos.x = tipRect.x + tipPad;
        tipPos.y = tipRect.y + tipPad;

        //if (tipRect.x + tipRect.height

        DrawRectangleRec(tipRect, Fade(BLACK, 0.8));
        DrawRectangleLinesEx(tipRect, 1, BLACK);
        DrawTextEx(
            fntHackBold20,
            tipStr,
            tipPos,
            fntHackBold20.baseSize,
            1,
            WHITE
        );
    }
}