#pragma once
#include "common.h"
#include "../common/strings.h"

struct TextureCatalog {
    struct Entry {
        Image image;
        Texture texture;

        Entry(Image &image, Texture &texture)
            : image(image), texture(texture) {}
    };

    void Init(void);
    void Free(void);
    bool Load(StringId id);
    const Entry &GetEntry(StringId id);
    const Texture &GetTexture(StringId id);
    void Unload(StringId id);

private:
    std::vector<Entry> entries{};
    std::unordered_map<StringId, size_t> entriesById{};
};

extern TextureCatalog rnTextureCatalog;
