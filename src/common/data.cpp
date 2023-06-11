#include "data.h"

namespace data {
    GfxFile gfxFiles[] = {
        { GFX_FILE_NONE },
        // id                     texture path
        { GFX_FILE_CHR_MAGE,      "resources/mage.png" },
        { GFX_FILE_NPC_LILY,      "resources/lily.png" },
        { GFX_FILE_OBJ_CAMPFIRE,  "resources/campfire.png" },
        { GFX_FILE_PRJ_BULLET,    "resources/bullet.png" },
        { GFX_FILE_TIL_OVERWORLD, "resources/tiles32.png" },
        //{ GFX_FILE_TILES32, "resources/tiles32.png" },
    };

    SfxFile sfxFiles[] = {
        { SFX_FILE_NONE },
        // id                      sound file path
        { SFX_FILE_CAMPFIRE,       "resources/campfire.wav" },
        { SFX_FILE_FOOTSTEP_GRASS, "resources/footstep_grass.wav" },
        { SFX_FILE_FOOTSTEP_STONE, "resources/footstep_stone.wav" },
    };

    GfxFrame gfxFrames[] = {
        { GFX_FRAME_NONE },
        // id                       image file                x   y    w    h
        { GFX_FRAME_CHR_MAGE_E_0,   GFX_FILE_CHR_MAGE,        0,  0,  32,  64 },
        { GFX_FRAME_CHR_MAGE_W_0,   GFX_FILE_CHR_MAGE,       32,  0,  32,  64 },
        { GFX_FRAME_NPC_LILY_E_0,   GFX_FILE_NPC_LILY,        0,  0,  32,  64 },
        { GFX_FRAME_NPC_LILY_W_0,   GFX_FILE_NPC_LILY,       32,  0,  32,  64 },
        { GFX_FRAME_OBJ_CAMPFIRE_0, GFX_FILE_OBJ_CAMPFIRE,    0,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_1, GFX_FILE_OBJ_CAMPFIRE,  256,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_2, GFX_FILE_OBJ_CAMPFIRE,  512,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_3, GFX_FILE_OBJ_CAMPFIRE,  768,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_4, GFX_FILE_OBJ_CAMPFIRE, 1024,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_5, GFX_FILE_OBJ_CAMPFIRE, 1280,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_6, GFX_FILE_OBJ_CAMPFIRE, 1536,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_7, GFX_FILE_OBJ_CAMPFIRE, 1792,  0, 256, 256 },
        { GFX_FRAME_PRJ_BULLET_0,   GFX_FILE_PRJ_BULLET,      0,  0,   8,   8 },
        { GFX_FRAME_TIL_GRASS,      GFX_FILE_TIL_OVERWORLD,   0, 96,  32,  32 },
        { GFX_FRAME_TIL_STONE_PATH, GFX_FILE_TIL_OVERWORLD, 128,  0,  32,  32 },
        { GFX_FRAME_TIL_WALL,       GFX_FILE_TIL_OVERWORLD, 128, 32,  32,  32 },
        { GFX_FRAME_TIL_WATER_0,    GFX_FILE_TIL_OVERWORLD, 128, 96,  32,  32 },
    };

    GfxAnim gfxAnims[] = {
        { GFX_ANIM_NONE },
        // id                      sound effect       frmRate  frmCount  frmDelay    frames
        { GFX_ANIM_CHR_MAGE_E,     SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_CHR_MAGE_E_0 }},
        { GFX_ANIM_CHR_MAGE_W,     SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_CHR_MAGE_W_0 }},
        { GFX_ANIM_NPC_LILY_E,     SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_NPC_LILY_E_0 }},
        { GFX_ANIM_NPC_LILY_W,     SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_NPC_LILY_W_0 }},
        { GFX_ANIM_OBJ_CAMPFIRE,   SFX_FILE_CAMPFIRE,      60,        8,        4, { GFX_FRAME_OBJ_CAMPFIRE_0, GFX_FRAME_OBJ_CAMPFIRE_1, GFX_FRAME_OBJ_CAMPFIRE_2, GFX_FRAME_OBJ_CAMPFIRE_3, GFX_FRAME_OBJ_CAMPFIRE_4, GFX_FRAME_OBJ_CAMPFIRE_5, GFX_FRAME_OBJ_CAMPFIRE_6, GFX_FRAME_OBJ_CAMPFIRE_7 }},
        { GFX_ANIM_PRJ_BULLET,     SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_PRJ_BULLET_0 }},
        { GFX_ANIM_TIL_GRASS,      SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_GRASS }},
        { GFX_ANIM_TIL_STONE_PATH, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_STONE_PATH }},
        { GFX_ANIM_TIL_WALL,       SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_WALL }},
        { GFX_ANIM_TIL_WATER,      SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_WATER_0 }},
    };

    TileMat tileMats[] = {
        { TILE_MAT_NONE },
        // id             footstep sound
        { TILE_MAT_GRASS, SFX_FILE_FOOTSTEP_GRASS },
        { TILE_MAT_STONE, SFX_FILE_FOOTSTEP_STONE },
        { TILE_MAT_WATER, SFX_FILE_FOOTSTEP_GRASS },
    };

    TileType tileTypes[] = {
        { TILE_NONE },
        //id               gfx/anim                 material        flags
        { TILE_GRASS,      GFX_ANIM_TIL_GRASS,      TILE_MAT_GRASS, TILE_FLAG_WALK },
        { TILE_STONE_PATH, GFX_ANIM_TIL_STONE_PATH, TILE_MAT_STONE, TILE_FLAG_WALK },
        { TILE_WALL,       GFX_ANIM_TIL_WALL,       TILE_MAT_STONE, TILE_FLAG_COLLIDE },
        { TILE_WATER,      GFX_ANIM_TIL_WATER,      TILE_MAT_WATER, TILE_FLAG_SWIM },
    };

    void Init(void)
    {
        for (GfxFile &gfxFile : gfxFiles) {
            gfxFile.texture = LoadTexture(gfxFile.path);
        }
        for (SfxFile &sfxFile : sfxFiles) {
            sfxFile.sound = LoadSound(sfxFile.path);
        }

        // Generate checkerboard image in slot 0 as a placeholder for when other images fail to load
        Image placeholderImg = GenImageChecked(16, 16, 4, 4, MAGENTA, WHITE);
        if (placeholderImg.width) {
            Texture placeholderTex = LoadTextureFromImage(placeholderImg);
            if (placeholderTex.width) {
                gfxFiles[0].texture = placeholderTex;
                gfxFrames[0].gfx = GFX_FILE_NONE;
                gfxFrames[0].w = placeholderTex.width;
                gfxFrames[0].h = placeholderTex.height;
            } else {
                printf("[data] WARN: Failed to generate placeholder texture\n");
            }
            UnloadImage(placeholderImg);
        } else {
            printf("[data] WARN: Failed to generate placeholder image\n");
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

    const GfxFrame &GetSpriteFrame(const Sprite &sprite)
    {
        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        const GfxFrameId frameId = anim.frames[sprite.animFrame];
        const GfxFrame &frame = gfxFrames[frameId];
        return frame;
    }

    void UpdateSprite(Sprite &sprite, EntityType entityType, Vector2 velocity, double dt)
    {
        sprite.animAccum += dt;

        // TODO: Make this more general and stop taking in entityType.
        switch (entityType) {
            case Entity_Player: case Entity_NPC: {
                if (velocity.x > 0) {
                    sprite.dir = data::DIR_E;
                } else {
                    sprite.dir = data::DIR_W;
                }
                break;
            }
        }

        const GfxAnimId animId = sprite.anims[sprite.dir];
        const GfxAnim &anim = gfxAnims[animId];
        const double animFrameTime = (1.0 / anim.frameRate) * anim.frameDelay;
        if (sprite.animAccum >= animFrameTime) {
            sprite.animFrame++;
            if (sprite.animFrame >= anim.frameCount) {
                sprite.animFrame = 0;
            }
            sprite.animAccum -= animFrameTime;
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

    void DrawSprite(const Sprite &sprite, Vector2 pos)
    {
        const GfxFrame &frame = GetSpriteFrame(sprite);
        const GfxFile &file = gfxFiles[frame.gfx];
        const Rectangle frameRect{ (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h };
        pos.x = floor(pos.x);
        pos.y = floor(pos.y);
        DrawTextureRec(file.texture, frameRect, pos, WHITE);
    }
}