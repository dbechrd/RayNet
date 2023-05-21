#include "data.h"

namespace diabs {
    GfxFile gfxFiles[] = {
        { GFX_FILE_NONE },
        { GFX_FILE_CAMPFIRE, "resources/campfire.png", {} },
        //{ GFX_FILE_TILES32, "resources/tiles32.png", {} },
    };

    SfxFile sfxFiles[] = {
        { SFX_FILE_NONE },
        { SFX_FILE_CAMPFIRE, "resources/campfire.wav", {} },
    };

    GfxFrame gfxFrames[] = {
        { GFX_FRAME_NONE },
        { GFX_FRAME_CAMPFIRE_0, GFX_FILE_CAMPFIRE,    0, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_1, GFX_FILE_CAMPFIRE,  256, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_2, GFX_FILE_CAMPFIRE,  512, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_3, GFX_FILE_CAMPFIRE,  768, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_4, GFX_FILE_CAMPFIRE, 1024, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_5, GFX_FILE_CAMPFIRE, 1280, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_6, GFX_FILE_CAMPFIRE, 1536, 0, 256, 256 },
        { GFX_FRAME_CAMPFIRE_7, GFX_FILE_CAMPFIRE, 1792, 0, 256, 256 },
    };

    GfxAnim gfxAnims[] = {
        { GFX_ANIM_NONE },
        { GFX_ANIM_CAMPFIRE, SFX_FILE_CAMPFIRE, 8, 4, {
            GFX_FRAME_CAMPFIRE_0,
            GFX_FRAME_CAMPFIRE_1,
            GFX_FRAME_CAMPFIRE_2,
            GFX_FRAME_CAMPFIRE_3,
            GFX_FRAME_CAMPFIRE_4,
            GFX_FRAME_CAMPFIRE_5,
            GFX_FRAME_CAMPFIRE_6,
            GFX_FRAME_CAMPFIRE_7
        }},
    };

    Sprite campfire = { DIR_N, { GFX_ANIM_CAMPFIRE }, 0, 0 };

    void Init(void)
    {
        for (GfxFile &gfxFile : gfxFiles) {
            gfxFile.texture = LoadTexture(gfxFile.path);
        }
        for (SfxFile &sfxFile : sfxFiles) {
            sfxFile.sound = LoadSound(sfxFile.path);
        }
    }

    void Free(void)
    {
        for (GfxFile &gfxFile : gfxFiles) {
            UnloadTexture(gfxFile.texture);
        }
        for (SfxFile &sfxFile : sfxFiles) {
            UnloadSound(sfxFile.sound);
        }
    }

    Vector2 GetSpriteSize(Sprite &sprite)
    {
        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        const GfxFrameId frameId = anim.frames[sprite.animFrame];
        const GfxFrame &frame = gfxFrames[frameId];
        const Vector2 size = { (float)frame.w, (float)frame.h };
        return size;
    }

    void UpdateSprite(Sprite &sprite)
    {
        sprite.animAccum++;

        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        if (sprite.animAccum >= anim.frameDelay) {
            sprite.animFrame++;
            if (sprite.animFrame >= anim.frameCount) {
                sprite.animFrame = 0;
            }
            sprite.animAccum = 0;
        }

        if (anim.sound) {
            const SfxFile &sfxFile = sfxFiles[anim.sound];
            if (!IsSoundPlaying(sfxFile.sound)) {
                PlaySound(sfxFile.sound);
            }
        }
    }

    void ResetSprite(Sprite &sprite)
    {
        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        if (anim.sound) {
            const SfxFile &sfxFile = sfxFiles[anim.sound];
            if (IsSoundPlaying(sfxFile.sound)) {
                StopSound(sfxFile.sound);
            }
        }

        sprite.animFrame = 0;
        sprite.animAccum = 0;
    }

    void DrawSprite(Sprite &sprite, Vector2 pos)
    {
        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        const GfxFrameId frameId = anim.frames[sprite.animFrame];
        const GfxFrame &frame = gfxFrames[frameId];
        const GfxFile &file = gfxFiles[frame.gfx];

        const Rectangle frameRect{
            (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h
        };
        printf("%.2f, %.2f, %.2f, %.2f\n", frameRect.x, frameRect.y, frameRect.width, frameRect.height);
        DrawTextureRec(file.texture, frameRect, pos, WHITE);
    }
}