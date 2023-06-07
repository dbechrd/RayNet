#include "histogram.h"
#include "collision.h"

Histogram histogram;

void Histogram::Push(Entry &entry)
{
    if (!paused) {
        buffer.push(entry);
    }
}

void Histogram::Draw(Vector2 position)
{
    float maxValue = 0.0f;
    for (int i = 0; i < buffer.size(); i++) {
        maxValue = MAX(maxValue, buffer[i].playerXDelta);
    }

    const float barScale = histoHeight / maxValue;

    Vector2 uiCursor{ position };

    const Vector2 mousePos = GetMousePosition();

    int hoverIdx = -1;

    for (int i = 0; i < buffer.size(); i++) {
        float bottom = uiCursor.y + histoHeight;
        float height = buffer[i].playerXDelta * barScale;

        Rectangle rect{};
        rect.x = uiCursor.x;
        rect.y = bottom - height;
        rect.width = barWidth;
        rect.height = height;
        DrawRectangleRec(rect, buffer[i].netTicked ? GREEN : RAYWHITE);

        // Including padding between rects in collision to prevent tooltip from
        // disappearing when exactly on a boundary causing flickering.
        Rectangle colRect = rect;
        colRect.x -= barPadding;
        colRect.width += barPadding;
        if (dlb_CheckCollisionPointRec(mousePos, colRect)) {
            Rectangle hover = rect;
            hover.y = position.y;
            hover.height = histoHeight;
            //hover.x -= 1;
            //hover.width += 2;
            DrawRectangleLinesEx(hover, 1, ORANGE);
            hoverIdx = i;
        }

        uiCursor.x += barWidth + barPadding;
    }

    if (hoverIdx >= 0) {
        Entry &entry = buffer[hoverIdx];
        const float tipPad = 8;
        Vector2 tipPos = Vector2Add(mousePos, { 12 + tipPad, 12 + tipPad });
        const char *tipStr = TextFormat(
            "frame                 %u\n"
            "now                   %.3f\n"
            "frameDt               %.3f\n"
            "lastInputSampledAt    %.3f\n"
            "lastProcessedInputSeq %u\n"
            "playerPos             %.2f, %.2f\n"
            "playerXDelta          %.2f",
            entry.frame,
            entry.now,
            entry.frameDt,
            entry.lastInputSampledAt,
            entry.lastProcessedInputCmd,
            entry.playerX,
            entry.playerY,
            entry.playerXDelta
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