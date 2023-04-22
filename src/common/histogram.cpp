#include "histogram.h"

void Histogram::Push(double value, Color color)
{
    if (!paused) {
        Entry entry{ value, color };
        buffer.push(entry);
    }
}

void Histogram::Draw(float x, float y)
{
    float maxValue = 0.0f;
    for (int i = 0; i < buffer.size(); i++) {
        maxValue = MAX(maxValue, buffer[i].value);
    }

    const float barScale = histoHeight / maxValue;

    Vector2 cursor{};
    cursor.x = x;
    cursor.y = y;

    for (int i = 0; i < buffer.size(); i++) {
        float bottom = cursor.y + histoHeight;
        float height = buffer[i].value * barScale;

        Rectangle rect{};
        rect.x = cursor.x;
        rect.y = bottom - height;
        rect.width = barWidth;
        rect.height = height;
        DrawRectangleRec(rect, buffer[i].color);

        cursor.x += barWidth + barPadding;
    }
}