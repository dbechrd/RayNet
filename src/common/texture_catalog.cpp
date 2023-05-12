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
    textures.push_back(missingTex);
    UnloadImage(missingImg);
}

void TextureCatalog::Free(void)
{
    for (auto &tex: textures) {
        UnloadTexture(tex);
    }
    textures.clear();
    textureIdByPath.clear();
}

TextureId TextureCatalog::FindOrLoad(const char *path)
{
    TextureId id = 0;
    const auto &existingTex = textureIdByPath.find(path);
    if (existingTex != textureIdByPath.end()) {
        id = existingTex->second;
    } else {
        Texture texture = LoadTexture(path);
        if (texture.width) {
            id = textures.size();
            textures.push_back(texture);
            textureIdByPath[path] = id;
        }
    }
    return id;
}

const Texture &TextureCatalog::ById(TextureId id)
{
    if (id >= 0 && id < textures.size()) {
        return textures[id];
    }
    return textures[0];
}

void TextureCatalog::Unload(const char *path)
{
    // TODO: unload it eventually (ref count?)
}