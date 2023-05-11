#include "audio.h"

RN_SoundSystem rnSoundSystem;

void RN_SoundSystem::Init(void)
{
    Load(RN_Sound_Tick_Soft, "resources/soft_tick.wav", 0.03f);
    Load(RN_Sound_Tick_Hard, "resources/hard_tick.wav", 0.03f);
    Load(RN_Sound_Lily_Introduction, "resources/mic_test.wav");

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
        if (rnSound.sound.frameCount) {
            rnSound.filename = 0;
            UnloadSound(rnSound.sound);
        }
    }
}

void RN_SoundSystem::Load(RN_SoundType soundType, const char *filename, float pitchVariance)
{
    RN_Sound &rnSound = rnSounds[soundType];
    if (!rnSound.filename) {
        Sound sound = LoadSound(filename);
        if (sound.frameCount) {
            rnSound.filename = filename;
            rnSound.sound = sound;
            rnSound.pitchVariance = pitchVariance;
        } else {
            printf("[sound_system] Failed to load sound %s\n", filename);
        }
    }
}

void RN_SoundSystem::Play(RN_SoundType soundType, bool multi, float pitchVariance)
{
    RN_Sound &rnSound = rnSounds[soundType];

    float variance = pitchVariance ? pitchVariance : rnSound.pitchVariance;
    SetSoundPitch(rnSound.sound, 1.0f + GetRandomFloatVariance(variance));

    if (multi) {
        PlaySoundMulti(rnSound.sound);
    } else if (!IsSoundPlaying(rnSound.sound)) {
        PlaySound(rnSound.sound);
    }
}
