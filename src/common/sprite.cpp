#include "sprite.h"

Vector2 Sprite::GetSize(void)
{
    const Spritesheet &spritesheet = rnSpritesheetCatalog.GetSpritesheet(spritesheetId);
    const Vector2 size{
        (float)spritesheet.frameWidth,
        (float)spritesheet.frameHeight
    };
    return size;
}

void Sprite::Draw(Vector2 pos, float scale, double now)
{
    // Position (after scaling)
    //float x = position.x;
    //float y = position.y - (size.y - sy) / 2;

    const Spritesheet &spritesheet = rnSpritesheetCatalog.GetSpritesheet(spritesheetId);
    const Texture &texture = rnTextureCatalog.GetTexture(spritesheet.textureId);
    const Animation &animation = spritesheet.animations[animationId];

    if (now - frameStartedAt >= animation.frameDuration && (frame < animation.frameCount || animation.loop)) {
        frame++;
        if (frame >= animation.frameCount) {
            frame = 0;
        }
        frameStartedAt = now;
    }

    int framesPerRow = texture.width / spritesheet.frameWidth;
    int absoluteFrame = animation.frameStart + frame;
    int frameX = absoluteFrame % framesPerRow;
    int frameY = absoluteFrame / framesPerRow;
    int x = frameX * spritesheet.frameWidth;
    int y = frameY * spritesheet.frameHeight;
    Rectangle frameRect{
        (float)x,
        (float)y,
        (float)spritesheet.frameWidth,
        (float)spritesheet.frameHeight
    };
    Rectangle dest{
        pos.x,
        pos.y,
        frameRect.width * scale,
        frameRect.height * scale
    };
    DrawTexturePro(texture, frameRect, dest, {}, 0, WHITE);
}