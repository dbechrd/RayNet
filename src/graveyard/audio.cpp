#include "audio.h"

SoundCatalog rnSoundCatalog;
data::MusFileId rnBackgroundMusic;

void SoundCatalog::Init(void)
{
    // TODO: Load default sound at index 0

    //Load(STR_SND_HARD_TICK, 0.03f);
    //Load(STR_SND_SOFT_TICK, 0.03f);
    //Load(STR_SND_MIC_TEST);
}

void SoundCatalog::Free(void)
{
    for (const auto &entry : entries) {
        UnloadSound(entry.sound);
    }
    entries.clear();
    entriesById.clear();
}

void SoundCatalog::Load(StringId id, float pitchVariance)
{
    size_t entryIdx = entriesById[id];
    if (!entryIdx) {
        const std::string &filename = rnStringCatalog.GetString(id);
        Sound sound = LoadSound(filename.c_str());
        if (sound.frameCount) {
            Entry entry{};
            entry.id = id;
            entry.sound = sound;
            entry.pitchVariance = pitchVariance;

            entryIdx = entries.size();
            entries.push_back(entry);
            entriesById[id] = entryIdx;
        } else {
            printf("[sound_system] Failed to load sound %s\n", filename.c_str());
        }
    }
}

const SoundCatalog::Entry &SoundCatalog::GetEntry(StringId id)
{
    const auto &entry = entriesById.find(id);
    if (entry != entriesById.end()) {
        size_t entryIdx = entry->second;
        if (entryIdx >= 0 && entryIdx < entries.size()) {
            return entries[entryIdx];
        }
    }
    return entries[0];
}

void SoundCatalog::Play(StringId id, bool multi, float pitchVariance)
{
    const Entry &entry = GetEntry(id);

    float variance = pitchVariance ? pitchVariance : entry.pitchVariance;
    SetSoundPitch(entry.sound, 1.0f + GetRandomFloatVariance(variance));

    if (multi) {
        PlaySoundMulti(entry.sound);
    } else if (!IsSoundPlaying(entry.sound)) {
        PlaySound(entry.sound);
    }
}
