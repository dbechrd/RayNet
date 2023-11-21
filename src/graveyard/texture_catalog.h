#pragma once
#include "common.h"
#include "../common/strings.h"

struct TextureCatalog {
    struct Entry {
        Image image;
        Texture texture;

        Entry(Image &image, Texture &texture) : image(image), texture(texture) {}
    };

    void Init(void);
    void Free(void);
    Err Load(RNString str);
    const Entry &GetEntry(RNString str);
    const Texture &GetTexture(RNString str);
    void Unload(RNString str);

private:
    std::vector<Entry> entries{};
    std::unordered_map<RNString, size_t, RNString::Hasher> entriesById{};
};

extern TextureCatalog rnTextureCatalog;
