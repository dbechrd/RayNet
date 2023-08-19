#include "texture_catalog.h"

TextureCatalog rnTextureCatalog;

void TextureCatalog::Init(void)
{
    // If this fails, we're doomed. There's probably no graphics device or something wack af.
    Image missingImg = GenImageChecked(32, 32, 4, 4, MAGENTA, WHITE);
    if (!missingImg.width) {
        printf("failed to generate placeholder image\n");
        exit(EXIT_FAILURE);
    }
    Texture missingTex = LoadTextureFromImage(missingImg);
    if (!missingTex.width) {
        printf("failed to generate placeholder texture\n");
        exit(EXIT_FAILURE);
    }

    Entry entry{ missingImg, missingTex };
    size_t entryIdx = entries.size();
    entries.push_back(entry);
    entriesById[STR_NULL] = entryIdx;
}

void TextureCatalog::Free(void)
{
    for (auto &entry: entries) {
        UnloadImage(entry.image);
        UnloadTexture(entry.texture);
    }
}

Err TextureCatalog::Load(StringId id)
{
    Err err = RN_SUCCESS;

    const auto &entry = entriesById.find(id);
    do {
        if (entry != entriesById.end()) {
            break;  // already loaded
        }

        std::string path = rnStringCatalog.GetString(id);
        Image image = LoadImage(path.c_str());
        if (!image.width) {
            UnloadImage(image);
            err = RN_BAD_FILE_READ; break;
        }

        Texture texture = LoadTextureFromImage(image);
        if (!texture.width) {
            err = RN_BAD_ALLOC; break;
        }

        Entry entry{ image, texture };
        size_t entryIdx = entries.size();
        entries.push_back(entry);
        entriesById[id] = entryIdx;
    } while (0);

    return err;
}

const TextureCatalog::Entry &TextureCatalog::GetEntry(StringId id)
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

const Texture &TextureCatalog::GetTexture(StringId id)
{
    Load(id);  // we don't care if this is successful because GetEntry returns placeholder if it fails
    const auto &entry = GetEntry(id);
    return entry.texture;
}

void TextureCatalog::Unload(StringId id)
{
    // TODO: unload it eventually (ref count?)
}