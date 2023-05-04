#pragma once
#include "../common.h"

enum RN_SoundType {
    RN_Sound_None,
    RN_Sound_Lily_Introduction = 1,

    RN_Sound_Count,
};

struct RN_Sound {
    const char *filename{};
    Sound sound{};
};

struct RN_SoundSystem {
    void Init(void);
    void Free(void);
    void Play(RN_SoundType soundType);

private:
    RN_Sound rnSounds[RN_Sound_Count]{};
};

extern RN_SoundSystem rnSoundSystem;
