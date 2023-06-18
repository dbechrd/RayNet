#pragma once
#include "common.h"
#include "strings.h"
#include "texture_catalog.h"

typedef int AnimationId;

struct Animation {
    StringId id;
    int frameStart;  // index into spritesheet
    int frameCount;
    float frameDuration;
    bool loop;
};

struct Spritesheet {
    int version;
    StringId textureId;
    int frameWidth;
    int frameHeight;
    std::vector<Animation> animations;  // TODO(dlb): Make this an unordered_map by StringId?

    Err Load(std::string path);
};

struct SpritesheetCatalog {
    void Init(void);
    void Free(void);
    void Load(StringId id);
    void Unload(StringId id);
    const Spritesheet &GetSpritesheet(StringId id);

private:
    std::vector<Spritesheet> entries{};
    std::unordered_map<StringId, size_t> entriesById{};
};

extern SpritesheetCatalog rnSpritesheetCatalog;
