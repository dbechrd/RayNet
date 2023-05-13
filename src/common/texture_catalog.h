#pragma once
#include "common.h"

typedef int TextureId;

struct TextureCatalogEntry {
    std::string path;
    Image image;
    Texture texture;

    TextureCatalogEntry(const char *path, Image &image, Texture &texture)
        : path(path), image(image), texture(texture) {}
};

struct TextureCatalog {
    void Init(void);
    void Free(void);
    TextureId FindOrLoad(const char *path);
    const TextureCatalogEntry &GetEntry(TextureId id);
    const Texture &GetTexture(TextureId id);
    void Unload(TextureId id);

private:
    std::vector<TextureCatalogEntry> entries{};
    std::unordered_map<std::string, TextureId> entriesByPath{};
};

extern TextureCatalog rnTextureCatalog;
