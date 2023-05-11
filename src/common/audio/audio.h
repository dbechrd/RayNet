#pragma once
#include "../common.h"

enum RN_SoundType {
    RN_Sound_None,

    RN_Sound_Tick_Soft,
    RN_Sound_Tick_Hard,
    RN_Sound_Lily_Introduction,

    RN_Sound_Count,
};

struct RN_Sound {
    const char *filename{};
    Sound sound{};
    float pitchVariance{};
};

struct RN_SoundSystem {
    void Init(void);
    void Free(void);
    void Load(RN_SoundType soundType, const char *filename, float pitchVariance = 0.0f);
    void Play(RN_SoundType soundType, bool multi = true, float pitchVariance = 0.0f);

private:
    RN_Sound rnSounds[RN_Sound_Count]{};
};

extern RN_SoundSystem rnSoundSystem;
