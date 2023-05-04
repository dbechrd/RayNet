#include "audio.h"

RN_SoundSystem rnSoundSystem;

void RN_SoundSystem::Init(void)
{
    rnSounds[RN_Sound_Lily_Introduction].filename = "resources/mic_test.wav";

    for (int i = 0; i < RN_Sound_Count; i++) {
        RN_Sound &rnSound = rnSounds[i];
        if (rnSound.filename) {
            rnSound.sound = LoadSound(rnSound.filename);
        }
    }
}

void RN_SoundSystem::Free(void)
{
    for (int i = 0; i < RN_Sound_Count; i++) {
        RN_Sound &rnSound = rnSounds[i];
        if (rnSound.filename) {
            UnloadSound(rnSound.sound);
        }
    }
}

void RN_SoundSystem::Play(RN_SoundType soundType)
{
    PlaySound(rnSounds[soundType].sound);
}
