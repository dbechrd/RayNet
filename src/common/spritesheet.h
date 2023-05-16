#pragma once
#include "common.h"
#include "texture_catalog.h"

typedef int AnimationId;
typedef int SpritesheetId;
typedef int SpriteId;

struct Animation {
    std::string name;
    int frameStart;  // index into spritesheet
    int frameCount;
    float frameDuration;
    bool loop;
};

struct Spritesheet {
    std::string filename;
    int version;
    TextureId textureId;
    int frameWidth;
    int frameHeight;
    std::vector<Animation> animations;

    Err Load(std::string path);
};

struct Sprite {
    SpritesheetId spritesheetId;
    AnimationId animationId;
    int frame;
    double frameStartedAt;

    void Draw(Vector2 pos, double now);
};

struct SpritesheetCatalog {
    void Init(void);
    void Free(void);
    SpritesheetId FindOrLoad(std::string path);
    const Spritesheet &GetSpritesheet(SpritesheetId id);
    void Unload(SpritesheetId id);

private:
    std::vector<Spritesheet> entries{};
    std::unordered_map<std::string, SpritesheetId> entriesByPath{};
};

extern SpritesheetCatalog rnSpritesheetCatalog;
