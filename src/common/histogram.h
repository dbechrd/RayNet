#pragma once
#include "common.h"
#include "ring_buffer.h"

struct Histogram {
    struct Entry {
        double value;
        Color color;
    };

    bool paused = false;
    float barPadding = 1.0f;
    float barWidth = 2.0f;
    float histoHeight = 20.0f;
    RingBuffer<Entry, 64> buffer;

    void Push(double value, Color color);
    void Draw(Vector2 position);
};