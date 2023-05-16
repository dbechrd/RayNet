#include "texture_catalog.h"

TextureCatalog rnTextureCatalog;

void TextureCatalog::Init(void)
{
    // If this fails, we're doomed. There's probably no graphics device or something wack af.
    Image missingImg = GenImageChecked(32, 32, 8, 8, MAGENTA, PURPLE);
    if (!missingImg.width) {
        printf("failed to generate placeholder image\n");
        exit(EXIT_FAILURE);
    }
    Texture missingTex = LoadTextureFromImage(missingImg);
    if (!missingTex.width) {
        printf("failed to generate placeholder texture\n");
        exit(EXIT_FAILURE);
    }

    const char *missingPath = "PLACEHOLDER";
    size_t textureId = entries.size();
    entries.emplace_back(missingPath, missingImg, missingTex);
    entriesByPath[missingPath] = textureId;
}

void TextureCatalog::Free(void)
{
    for (auto &entry: entries) {
        UnloadImage(entry.image);
        UnloadTexture(entry.texture);
    }
}

TextureId TextureCatalog::FindOrLoad(std::string path)
{
    TextureId id = 0;
    const auto &entry = entriesByPath.find(path);
    if (entry != entriesByPath.end()) {
        id = entry->second;
    } else {
        Image image = LoadImage(path.c_str());
        if (image.width) {
            Texture texture = LoadTextureFromImage(image);
            if (texture.width) {
                id = entries.size();
                entries.emplace_back(path, image, texture);
                entriesByPath[path] = id;
            }
        } else {
            // Not really necessary, but seems weird to have a image when texture load fails?
            UnloadImage(image);
        }
    }
    return id;
}

const TextureCatalogEntry &TextureCatalog::GetEntry(TextureId id)
{
    if (id >= 0 && id < entries.size()) {
        return entries[id];
    }
    return entries[0];
}

const Texture &TextureCatalog::GetTexture(TextureId id)
{
    return GetEntry(id).texture;
}

void TextureCatalog::Unload(TextureId id)
{
    // TODO: unload it eventually (ref count?)
}