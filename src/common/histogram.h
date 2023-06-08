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
        float value2{}; // useful for dt from previous
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

    static Histogram *hoveredHisto;
    static int hoveredIdx;

    RingBuffer<Entry, 60> buffer;

    void Push(Entry &entry);
    static void ResetHover(void);
    void Draw(Vector2 position);
    static void DrawHover(void);
};

extern Histogram histoFps;
extern Histogram histoInput;
extern Histogram histoDx;
