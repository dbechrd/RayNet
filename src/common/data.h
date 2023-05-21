#pragma once
#include "common.h"

////////////////////////////////////////////////////////////////////////////
// TileFrame: textureId, x, y, w, h
// TileGraphic: uint8_t id, const char *name, animLen[8], frames[8], delays[8] (up to 8 anims, for each direction)
// TileMaterial: flags (walk, swim), footstepSoundId
// TileType: uint8_t id, tileGraphicId, tileMaterialId
////////////////////////////////////////////////////////////////////////////

namespace diabs {
    enum Direction : uint8_t {
        DIR_N,
        DIR_E,
        DIR_S,
        DIR_W,
        DIR_NE,
        DIR_SE,
        DIR_SW,
        DIR_NW
    };
    enum GfxFileId : uint16_t {
        GFX_FILE_NONE,
        GFX_FILE_CAMPFIRE,
        //GFX_FILE_TILES32,
    };
    struct GfxFile {
        GfxFileId id;
        const char *path;
        ::Texture texture;
    };

    enum SfxFileId : uint16_t {
        SFX_FILE_NONE,
        SFX_FILE_CAMPFIRE,
    };
    struct SfxFile {
        SfxFileId id;
        const char *path;
        ::Sound sound;
    };

    enum GfxFrameId : uint16_t {
        GFX_FRAME_NONE,
        GFX_FRAME_CAMPFIRE_0,
        GFX_FRAME_CAMPFIRE_1,
        GFX_FRAME_CAMPFIRE_2,
        GFX_FRAME_CAMPFIRE_3,
        GFX_FRAME_CAMPFIRE_4,
        GFX_FRAME_CAMPFIRE_5,
        GFX_FRAME_CAMPFIRE_6,
        GFX_FRAME_CAMPFIRE_7,
        //GFX_FRAME_WATER_0,
        //GFX_FRAME_WATER_1,
        //GFX_FRAME_FLAG_0,
        //GFX_FRAME_FLAG_1,
        //GFX_FRAME_FLAG_2,
        //GFX_FRAME_FLAG_3,
    };
    struct GfxFrame {
        GfxFrameId id;
        GfxFileId gfx;
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
    };

    enum GfxAnimId : uint16_t {
        GFX_ANIM_NONE,
        GFX_ANIM_CAMPFIRE,
        //GFX_ANIM_LILY_N,
        //GFX_ANIM_LILY_E,
        //GFX_ANIM_LILY_S,
        //GFX_ANIM_LILY_W,
        //GFX_ANIM_FLAG,
    };
    struct GfxAnim {
        GfxAnimId id;
        SfxFileId sound;
        uint8_t frameCount;
        uint8_t frameDelay;
        GfxFrameId frames[8];
    };

    struct Sprite {
        Direction dir;
        GfxAnimId anims[8];  // for each direction
        uint8_t animFrame; // current frame index
        uint8_t animAccum; // frames since last update
    };

    //enum TileTypeId {
    //    TILE_TYPE_WATER,
    //    TILE_TYPE_DIRT,
    //    TILE_TYPE_STONE_PATH,
    //    TILE_TYPE_GRASS,
    //    TILE_TYPE_GRASS_FLOWERS,
    //};

    extern GfxFile gfxFiles[];
    extern SfxFile sfxFiles[];
    extern GfxFrame gfxFrames[];
    extern GfxAnim gfxAnims[];
    extern Sprite campfire;

    void Init(void);
    void Free(void);

    Vector2 GetSpriteSize(Sprite &sprite);
    void UpdateSprite(Sprite &sprite);
    void ResetSprite(Sprite &sprite);
    void DrawSprite(Sprite &sprite, Vector2 pos);
}
////////////////////////////////////////////////////////////////////////////