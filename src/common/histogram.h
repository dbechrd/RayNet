#pragma once
#include "common.h"
#include "ring_buffer.h"

struct Histogram {
    struct Entry {
        // Client and server
        uint64_t frame{};
        double now{};

#if 0
        double frameDt{};
        bool netTicked{};

        // Just client
        double lastInputSampledAt{};
        uint32_t lastProcessedInputCmd{};
        float playerX{};
        float playerXDelta{};
#endif

        // TODO: Make multiple histograms using this general structure:
        float value{};
        Color color{};
        std::string metadata{};

        Entry(void) = default;
        Entry(uint64_t frame, double now)
            : frame(frame), now(now) {}
    };

    static const float barPadding;
    static const float barWidth;
    static const float histoHeight;
    static bool paused;

    Histogram *hoveredHisto{};
    int hoveredIdx = -1;

    RingBuffer<Entry, 238> buffer;

    void Push(Entry &entry);
    void Draw(Vector2 position);
    void DrawHover(void);
};

extern Histogram histoFps;
extern Histogram histoInput;
extern Histogram histoDx;
