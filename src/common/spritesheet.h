#pragma once
#include "common.h"
#include "texture_catalog.h"

typedef int SpriteId;

struct Animation {
    std::string name;
    int frameStart;  // index into spritesheet
    int frameCount;
    bool loop;
};

struct Spritesheet {
    int version;
    TextureId textureId;
    int frameWidth;
    int frameHeight;
    float frameDuration;
    std::vector<Animation> animations;

    Err LoadAsSingleRowAnimation(std::string path);
};

//struct Sprite {
//    int width;
//    int height;
//};