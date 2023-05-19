#pragma once
#include "../common.h"
#include "../strings.h"

struct SoundCatalog {
    struct Entry {
        StringId id{};
        Sound sound{};
        float pitchVariance{};
    };

    void Init(void);
    void Free(void);
    void Load(StringId id, float pitchVariance = 0.0f);
    void Play(StringId id, bool multi = true, float pitchVariance = 0.0f);

private:
    const SoundCatalog::Entry &GetEntry(StringId id);

    std::vector<Entry> entries{};
    std::unordered_map<StringId, size_t> entriesById{};
};

extern SoundCatalog rnSoundCatalog;
