#pragma once
#include "common.h"

typedef int TextureId;

struct TextureCatalog {
    void Init(void);
    void Free(void);
    TextureId FindOrLoad(const char *path);
    const Texture &ById(TextureId id);
    void Unload(const char *path);

private:
    std::vector<Texture> textures{};
    std::unordered_map<std::string, TextureId> textureIdByPath{};
};

extern TextureCatalog rnTextureCatalog;
