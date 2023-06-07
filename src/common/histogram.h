#pragma once
#include "common.h"
#include "ring_buffer.h"

struct Histogram {
    struct Entry {
        // Client and server
        uint64_t frame{};
        double now{};
        double frameDt{};
        bool netTicked{};

        // Just client
        double lastInputSampledAt{};
        uint32_t lastProcessedInputCmd{};
        float playerX{};
        float playerY{};
        float playerXDelta{};

        // TODO: Make multiple histograms using this general structure:
        //std::string metadata{};
        //float value{};
        //Color color{};

        Entry(void) = default;
        Entry(uint64_t frame, double now, double frameDt, bool netTicked)
            : frame(frame), now(now), frameDt(frameDt), netTicked(netTicked) {}
    };

    bool paused = false;
    float barPadding = 1.0f;
    float barWidth = 7.0f;
    float histoHeight = 40.0f;
    RingBuffer<Entry, 60> buffer;

    void Push(Entry &entry);
    void Draw(Vector2 position);
};

extern Histogram histogram;
