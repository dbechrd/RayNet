#pragma once
#include "common.h"

enum EntityType;

////////////////////////////////////////////////////////////////////////////
// TileFrame: textureId, x, y, w, h
// TileGraphic: uint8_t id, const char *name, animLen[8], frames[8], delays[8] (up to 8 anims, for each direction)
// TileMaterial: flags (walk, swim), footstepSoundId
// TileType: uint8_t id, tileGraphicId, tileMaterialId
////////////////////////////////////////////////////////////////////////////

namespace data {
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
        GFX_FILE_DLG_NPATCH,
        GFX_FILE_CHR_MAGE,
        GFX_FILE_NPC_LILY,
        GFX_FILE_OBJ_CAMPFIRE,
        GFX_FILE_PRJ_BULLET,
        GFX_FILE_TIL_OVERWORLD,
        GFX_FILE_TIL_AUTO_GRASS,
    };
    struct GfxFile {
        GfxFileId id;
        const char *path;
        ::Texture texture;
    };

    enum MusFileId : uint16_t {
        MUS_FILE_NONE,
        MUS_FILE_AMBIENT_OUTDOORS,
        MUS_FILE_AMBIENT_CAVE,
    };
    struct MusFile {
        MusFileId id;
        const char *path;
        ::Music music;
    };

    enum SfxFileId : uint16_t {
        SFX_FILE_NONE,
        SFX_FILE_SOFT_TICK,
        SFX_FILE_CAMPFIRE,
        SFX_FILE_FOOTSTEP_GRASS,
        SFX_FILE_FOOTSTEP_STONE,
    };
    struct SfxFile {
        SfxFileId id;
        const char *path;
        float pitch_variance;
        ::Sound sound;
    };

    enum GfxFrameId : uint16_t {
        GFX_FRAME_NONE,
        GFX_FRAME_CHR_MAGE_E_0,
        GFX_FRAME_CHR_MAGE_W_0,
        GFX_FRAME_NPC_LILY_E_0,
        GFX_FRAME_NPC_LILY_W_0,
        GFX_FRAME_OBJ_CAMPFIRE_0,
        GFX_FRAME_OBJ_CAMPFIRE_1,
        GFX_FRAME_OBJ_CAMPFIRE_2,
        GFX_FRAME_OBJ_CAMPFIRE_3,
        GFX_FRAME_OBJ_CAMPFIRE_4,
        GFX_FRAME_OBJ_CAMPFIRE_5,
        GFX_FRAME_OBJ_CAMPFIRE_6,
        GFX_FRAME_OBJ_CAMPFIRE_7,
        //GFX_FRAME_OBJ_FLAG_0,
        //GFX_FRAME_OBJ_FLAG_1,
        //GFX_FRAME_OBJ_FLAG_2,
        //GFX_FRAME_OBJ_FLAG_3,
        GFX_FRAME_PRJ_BULLET_0,
        GFX_FRAME_TIL_GRASS,
        GFX_FRAME_TIL_STONE_PATH,
        GFX_FRAME_TIL_WALL,
        GFX_FRAME_TIL_WATER_0,

        // TODO: Make a macro that generates these??
        GFX_FRAME_TIL_AUTO_GRASS_00,
        GFX_FRAME_TIL_AUTO_GRASS_01,
        GFX_FRAME_TIL_AUTO_GRASS_02,
        GFX_FRAME_TIL_AUTO_GRASS_03,
        GFX_FRAME_TIL_AUTO_GRASS_04,
        GFX_FRAME_TIL_AUTO_GRASS_05,
        GFX_FRAME_TIL_AUTO_GRASS_06,
        GFX_FRAME_TIL_AUTO_GRASS_07,
        GFX_FRAME_TIL_AUTO_GRASS_08,
        GFX_FRAME_TIL_AUTO_GRASS_09,
        GFX_FRAME_TIL_AUTO_GRASS_10,
        GFX_FRAME_TIL_AUTO_GRASS_11,
        GFX_FRAME_TIL_AUTO_GRASS_12,
        GFX_FRAME_TIL_AUTO_GRASS_13,
        GFX_FRAME_TIL_AUTO_GRASS_14,
        GFX_FRAME_TIL_AUTO_GRASS_15,
        GFX_FRAME_TIL_AUTO_GRASS_16,
        GFX_FRAME_TIL_AUTO_GRASS_17,
        GFX_FRAME_TIL_AUTO_GRASS_18,
        GFX_FRAME_TIL_AUTO_GRASS_19,
        GFX_FRAME_TIL_AUTO_GRASS_20,
        GFX_FRAME_TIL_AUTO_GRASS_21,
        GFX_FRAME_TIL_AUTO_GRASS_22,
        GFX_FRAME_TIL_AUTO_GRASS_23,
        GFX_FRAME_TIL_AUTO_GRASS_24,
        GFX_FRAME_TIL_AUTO_GRASS_25,
        GFX_FRAME_TIL_AUTO_GRASS_26,
        GFX_FRAME_TIL_AUTO_GRASS_27,
        GFX_FRAME_TIL_AUTO_GRASS_28,
        GFX_FRAME_TIL_AUTO_GRASS_29,
        GFX_FRAME_TIL_AUTO_GRASS_30,
        GFX_FRAME_TIL_AUTO_GRASS_31,
        GFX_FRAME_TIL_AUTO_GRASS_32,
        GFX_FRAME_TIL_AUTO_GRASS_33,
        GFX_FRAME_TIL_AUTO_GRASS_34,
        GFX_FRAME_TIL_AUTO_GRASS_35,
        GFX_FRAME_TIL_AUTO_GRASS_36,
        GFX_FRAME_TIL_AUTO_GRASS_37,
        GFX_FRAME_TIL_AUTO_GRASS_38,
        GFX_FRAME_TIL_AUTO_GRASS_39,
        GFX_FRAME_TIL_AUTO_GRASS_40,
        GFX_FRAME_TIL_AUTO_GRASS_41,
        GFX_FRAME_TIL_AUTO_GRASS_42,
        GFX_FRAME_TIL_AUTO_GRASS_43,
        GFX_FRAME_TIL_AUTO_GRASS_44,
        GFX_FRAME_TIL_AUTO_GRASS_45,
        GFX_FRAME_TIL_AUTO_GRASS_46,
        GFX_FRAME_TIL_AUTO_GRASS_47,
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
        GFX_ANIM_CHR_MAGE_E,
        GFX_ANIM_CHR_MAGE_W,
        GFX_ANIM_NPC_LILY_E,
        GFX_ANIM_NPC_LILY_W,
        GFX_ANIM_OBJ_CAMPFIRE,
        GFX_ANIM_PRJ_BULLET,
        //GFX_ANIM_FLAG,
        GFX_ANIM_TIL_GRASS,
        GFX_ANIM_TIL_STONE_PATH,
        GFX_ANIM_TIL_WALL,
        GFX_ANIM_TIL_WATER,

        GFX_ANIM_TIL_AUTO_GRASS_00,
        GFX_ANIM_TIL_AUTO_GRASS_01,
        GFX_ANIM_TIL_AUTO_GRASS_02,
        GFX_ANIM_TIL_AUTO_GRASS_03,
        GFX_ANIM_TIL_AUTO_GRASS_04,
        GFX_ANIM_TIL_AUTO_GRASS_05,
        GFX_ANIM_TIL_AUTO_GRASS_06,
        GFX_ANIM_TIL_AUTO_GRASS_07,
        GFX_ANIM_TIL_AUTO_GRASS_08,
        GFX_ANIM_TIL_AUTO_GRASS_09,
        GFX_ANIM_TIL_AUTO_GRASS_10,
        GFX_ANIM_TIL_AUTO_GRASS_11,
        GFX_ANIM_TIL_AUTO_GRASS_12,
        GFX_ANIM_TIL_AUTO_GRASS_13,
        GFX_ANIM_TIL_AUTO_GRASS_14,
        GFX_ANIM_TIL_AUTO_GRASS_15,
        GFX_ANIM_TIL_AUTO_GRASS_16,
        GFX_ANIM_TIL_AUTO_GRASS_17,
        GFX_ANIM_TIL_AUTO_GRASS_18,
        GFX_ANIM_TIL_AUTO_GRASS_19,
        GFX_ANIM_TIL_AUTO_GRASS_20,
        GFX_ANIM_TIL_AUTO_GRASS_21,
        GFX_ANIM_TIL_AUTO_GRASS_22,
        GFX_ANIM_TIL_AUTO_GRASS_23,
        GFX_ANIM_TIL_AUTO_GRASS_24,
        GFX_ANIM_TIL_AUTO_GRASS_25,
        GFX_ANIM_TIL_AUTO_GRASS_26,
        GFX_ANIM_TIL_AUTO_GRASS_27,
        GFX_ANIM_TIL_AUTO_GRASS_28,
        GFX_ANIM_TIL_AUTO_GRASS_29,
        GFX_ANIM_TIL_AUTO_GRASS_30,
        GFX_ANIM_TIL_AUTO_GRASS_31,
        GFX_ANIM_TIL_AUTO_GRASS_32,
        GFX_ANIM_TIL_AUTO_GRASS_33,
        GFX_ANIM_TIL_AUTO_GRASS_34,
        GFX_ANIM_TIL_AUTO_GRASS_35,
        GFX_ANIM_TIL_AUTO_GRASS_36,
        GFX_ANIM_TIL_AUTO_GRASS_37,
        GFX_ANIM_TIL_AUTO_GRASS_38,
        GFX_ANIM_TIL_AUTO_GRASS_39,
        GFX_ANIM_TIL_AUTO_GRASS_40,
        GFX_ANIM_TIL_AUTO_GRASS_41,
        GFX_ANIM_TIL_AUTO_GRASS_42,
        GFX_ANIM_TIL_AUTO_GRASS_43,
        GFX_ANIM_TIL_AUTO_GRASS_44,
        GFX_ANIM_TIL_AUTO_GRASS_45,
        GFX_ANIM_TIL_AUTO_GRASS_46,
        GFX_ANIM_TIL_AUTO_GRASS_47,
    };
    struct GfxAnim {
        GfxAnimId id;
        SfxFileId sound;
        uint8_t frameRate;
        uint8_t frameCount;
        uint8_t frameDelay;
        GfxFrameId frames[8];
    };

    struct Sprite {
        Direction dir;
        GfxAnimId anims[8];  // for each direction
        uint8_t animFrame; // current frame index
        double animAccum; // time since last update
    };

    enum TileMatId {
        TILE_MAT_NONE,
        TILE_MAT_GRASS,
        TILE_MAT_STONE,
        TILE_MAT_WALL,
        TILE_MAT_WATER,
    };

    struct TileMat {
        TileMatId id;
        SfxFileId footstepSnd;
    };

    enum TileTypeId {
        TILE_NONE,
        TILE_GRASS,
        TILE_STONE_PATH,
        TILE_WATER,
        TILE_WALL,

        TILE_AUTO_GRASS_00,
        TILE_AUTO_GRASS_01,
        TILE_AUTO_GRASS_02,
        TILE_AUTO_GRASS_03,
        TILE_AUTO_GRASS_04,
        TILE_AUTO_GRASS_05,
        TILE_AUTO_GRASS_06,
        TILE_AUTO_GRASS_07,
        TILE_AUTO_GRASS_08,
        TILE_AUTO_GRASS_09,
        TILE_AUTO_GRASS_10,
        TILE_AUTO_GRASS_11,
        TILE_AUTO_GRASS_12,
        TILE_AUTO_GRASS_13,
        TILE_AUTO_GRASS_14,
        TILE_AUTO_GRASS_15,
        TILE_AUTO_GRASS_16,
        TILE_AUTO_GRASS_17,
        TILE_AUTO_GRASS_18,
        TILE_AUTO_GRASS_19,
        TILE_AUTO_GRASS_20,
        TILE_AUTO_GRASS_21,
        TILE_AUTO_GRASS_22,
        TILE_AUTO_GRASS_23,
        TILE_AUTO_GRASS_24,
        TILE_AUTO_GRASS_25,
        TILE_AUTO_GRASS_26,
        TILE_AUTO_GRASS_27,
        TILE_AUTO_GRASS_28,
        TILE_AUTO_GRASS_29,
        TILE_AUTO_GRASS_30,
        TILE_AUTO_GRASS_31,
        TILE_AUTO_GRASS_32,
        TILE_AUTO_GRASS_33,
        TILE_AUTO_GRASS_34,
        TILE_AUTO_GRASS_35,
        TILE_AUTO_GRASS_36,
        TILE_AUTO_GRASS_37,
        TILE_AUTO_GRASS_38,
        TILE_AUTO_GRASS_39,
        TILE_AUTO_GRASS_40,
        TILE_AUTO_GRASS_41,
        TILE_AUTO_GRASS_42,
        TILE_AUTO_GRASS_43,
        TILE_AUTO_GRASS_44,
        TILE_AUTO_GRASS_45,
        TILE_AUTO_GRASS_46,
        TILE_AUTO_GRASS_47,
    };

    typedef uint32_t TileFlags;
    enum {
        TILE_FLAG_COLLIDE = 0x01,
        TILE_FLAG_WALK    = 0x02,
        TILE_FLAG_SWIM    = 0x04,
    };

    struct TileType {
        TileTypeId id;
        GfxAnimId anim;
        TileMatId material;
        uint8_t auto_tile_mask;
        TileFlags flags;
        //Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    };

    extern GfxFile gfxFiles[];
    extern MusFile musFiles[];
    extern SfxFile sfxFiles[];
    extern GfxFrame gfxFrames[];
    extern GfxAnim gfxAnims[];
    extern TileMat tileMats[];
    extern TileType tileTypes[];

    void Init(void);
    void Free(void);

    void PlaySound(SfxFileId id, bool multi = true, float pitchVariance = 0.0f);

    const GfxFrame &GetSpriteFrame(const Sprite &sprite);
    void UpdateSprite(Sprite &sprite, EntityType entityType, Vector2 velocity, double dt);
    void ResetSprite(Sprite &sprite);
    void DrawSprite(const Sprite &sprite, Vector2 pos);
}
////////////////////////////////////////////////////////////////////////////