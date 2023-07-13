#include "data.h"

namespace data {
#define ENUM_STR_GENERATOR(type, enumDef)     \
    const char *type##Str(type id) {          \
        switch (id) {                         \
            enumDef(ENUM_GEN_CASE_RETURN_STR) \
            default: return "<unknown>";      \
        }                                     \
    }

    ENUM_STR_GENERATOR(GfxFileId,  GFX_FILE_IDS);
    ENUM_STR_GENERATOR(MusFileId,  MUS_FILE_IDS);
    ENUM_STR_GENERATOR(SfxFileId,  SFX_FILE_IDS);
    ENUM_STR_GENERATOR(GfxFrameId, GFX_FRAME_IDS);
    ENUM_STR_GENERATOR(GfxAnimId,  GFX_ANIM_IDS);
    ENUM_STR_GENERATOR(MaterialId, MATERIAL_IDS);
    ENUM_STR_GENERATOR(TileTypeId, TILE_TYPE_IDS);

#undef ENUM_STR_GENERATOR

    GfxFile gfxFiles[] = {
        { GFX_FILE_NONE },
        // id                      texture path
        { GFX_FILE_DLG_NPATCH,     "resources/npatch.png" },
        { GFX_FILE_CHR_MAGE,       "resources/mage.png" },
        { GFX_FILE_NPC_LILY,       "resources/lily.png" },
        { GFX_FILE_OBJ_CAMPFIRE,   "resources/campfire.png" },
        { GFX_FILE_PRJ_BULLET,     "resources/bullet.png" },
        { GFX_FILE_TIL_OVERWORLD,  "resources/tiles32.png" },
        { GFX_FILE_TIL_AUTO_GRASS, "resources/autotile_3x3_min.png" },
    };

    MusFile musFiles[] = {
        { MUS_FILE_NONE },
        // id                        music file path
        { MUS_FILE_AMBIENT_OUTDOORS, "resources/copyright/345470__philip_goddard__branscombe-landslip-birds-and-sea-echoes-ese-from-cave-track.ogg" },
        { MUS_FILE_AMBIENT_CAVE,     "resources/copyright/69391__zixem__cave_amb.wav" },
    };

    SfxFile sfxFiles[] = {
        { SFX_FILE_NONE },
        // id                      sound file path                 pitch variance
        { SFX_FILE_SOFT_TICK,      "resources/soft_tick.wav"     , 0.03f },
        { SFX_FILE_CAMPFIRE,       "resources/campfire.wav"      , 0.00f },
        { SFX_FILE_FOOTSTEP_GRASS, "resources/footstep_grass.wav", 0.00f },
        { SFX_FILE_FOOTSTEP_STONE, "resources/footstep_stone.wav", 0.00f },
    };

    GfxFrame gfxFrames[] = {
        // id                       image file                x   y    w    h
        { GFX_FRAME_NONE },

        // playable characters
        { GFX_FRAME_CHR_MAGE_E_0,   GFX_FILE_CHR_MAGE,        0,  0,  32,  64 },
        { GFX_FRAME_CHR_MAGE_W_0,   GFX_FILE_CHR_MAGE,       32,  0,  32,  64 },

        // npcs
        { GFX_FRAME_NPC_LILY_E_0,   GFX_FILE_NPC_LILY,        0,  0,  32,  64 },
        { GFX_FRAME_NPC_LILY_W_0,   GFX_FILE_NPC_LILY,       32,  0,  32,  64 },

        // objects
        { GFX_FRAME_OBJ_CAMPFIRE_0, GFX_FILE_OBJ_CAMPFIRE,    0,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_1, GFX_FILE_OBJ_CAMPFIRE,  256,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_2, GFX_FILE_OBJ_CAMPFIRE,  512,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_3, GFX_FILE_OBJ_CAMPFIRE,  768,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_4, GFX_FILE_OBJ_CAMPFIRE, 1024,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_5, GFX_FILE_OBJ_CAMPFIRE, 1280,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_6, GFX_FILE_OBJ_CAMPFIRE, 1536,  0, 256, 256 },
        { GFX_FRAME_OBJ_CAMPFIRE_7, GFX_FILE_OBJ_CAMPFIRE, 1792,  0, 256, 256 },

        // projectiles
        { GFX_FRAME_PRJ_BULLET_0,   GFX_FILE_PRJ_BULLET,      0,  0,   8,   8 },

        // tiles
        { GFX_FRAME_TIL_GRASS,      GFX_FILE_TIL_OVERWORLD,   0, 96,  32,  32 },
        { GFX_FRAME_TIL_STONE_PATH, GFX_FILE_TIL_OVERWORLD, 128,  0,  32,  32 },
        { GFX_FRAME_TIL_WALL,       GFX_FILE_TIL_OVERWORLD, 128, 32,  32,  32 },
        { GFX_FRAME_TIL_WATER_0,    GFX_FILE_TIL_OVERWORLD, 128, 96,  32,  32 },

        { GFX_FRAME_TIL_AUTO_GRASS_00, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  0, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_01, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  0, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_02, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  0, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_03, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  0, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_04, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  1, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_05, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  1, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_06, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  1, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_07, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  1, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_08, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  2, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_09, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  2, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_10, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  2, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_11, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  2, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_12, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  3, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_13, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  3, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_14, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  3, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_15, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  3, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_16, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  4, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_17, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  4, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_18, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  4, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_19, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  4, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_20, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  5, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_21, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  5, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_22, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  5, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_23, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  5, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_24, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  6, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_25, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  6, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_26, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  6, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_27, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  6, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_28, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  7, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_29, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  7, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_30, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  7, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_31, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  7, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_32, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  8, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_33, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  8, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_34, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  8, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_35, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  8, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_36, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W *  9, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_37, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W *  9, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_38, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W *  9, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_39, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W *  9, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_40, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W * 10, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_41, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W * 10, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_42, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W * 10, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_43, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W * 10, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_44, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 0, TILE_W * 11, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_45, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 1, TILE_W * 11, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_46, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 2, TILE_W * 11, TILE_W, TILE_W },
        { GFX_FRAME_TIL_AUTO_GRASS_47, GFX_FILE_TIL_AUTO_GRASS, TILE_W * 3, TILE_W * 11, TILE_W, TILE_W },
    };

    GfxAnim gfxAnims[] = {
        // id                         sound effect       frmRate  frmCount  frmDelay    frames
        { GFX_ANIM_NONE },

        // playable characters
        { GFX_ANIM_CHR_MAGE_E,        SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_CHR_MAGE_E_0 }},
        { GFX_ANIM_CHR_MAGE_W,        SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_CHR_MAGE_W_0 }},

        // npcs
        { GFX_ANIM_NPC_LILY_E,        SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_NPC_LILY_E_0 }},
        { GFX_ANIM_NPC_LILY_W,        SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_NPC_LILY_W_0 }},

        // objects
        { GFX_ANIM_OBJ_CAMPFIRE,      SFX_FILE_CAMPFIRE,      60,        8,        4, { GFX_FRAME_OBJ_CAMPFIRE_0, GFX_FRAME_OBJ_CAMPFIRE_1, GFX_FRAME_OBJ_CAMPFIRE_2, GFX_FRAME_OBJ_CAMPFIRE_3, GFX_FRAME_OBJ_CAMPFIRE_4, GFX_FRAME_OBJ_CAMPFIRE_5, GFX_FRAME_OBJ_CAMPFIRE_6, GFX_FRAME_OBJ_CAMPFIRE_7 }},

        // projectiles
        { GFX_ANIM_PRJ_BULLET,        SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_PRJ_BULLET_0 }},

        // tiles
        { GFX_ANIM_TIL_GRASS,         SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_GRASS }},
        { GFX_ANIM_TIL_STONE_PATH,    SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_STONE_PATH }},
        { GFX_ANIM_TIL_WALL,          SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_WALL }},
        { GFX_ANIM_TIL_WATER,         SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_WATER_0 }},

        { GFX_ANIM_TIL_AUTO_GRASS_00, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_00 }},
        { GFX_ANIM_TIL_AUTO_GRASS_01, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_01 }},
        { GFX_ANIM_TIL_AUTO_GRASS_02, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_02 }},
        { GFX_ANIM_TIL_AUTO_GRASS_03, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_03 }},
        { GFX_ANIM_TIL_AUTO_GRASS_04, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_04 }},
        { GFX_ANIM_TIL_AUTO_GRASS_05, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_05 }},
        { GFX_ANIM_TIL_AUTO_GRASS_06, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_06 }},
        { GFX_ANIM_TIL_AUTO_GRASS_07, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_07 }},
        { GFX_ANIM_TIL_AUTO_GRASS_08, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_08 }},
        { GFX_ANIM_TIL_AUTO_GRASS_09, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_09 }},
        { GFX_ANIM_TIL_AUTO_GRASS_10, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_10 }},
        { GFX_ANIM_TIL_AUTO_GRASS_11, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_11 }},
        { GFX_ANIM_TIL_AUTO_GRASS_12, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_12 }},
        { GFX_ANIM_TIL_AUTO_GRASS_13, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_13 }},
        { GFX_ANIM_TIL_AUTO_GRASS_14, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_14 }},
        { GFX_ANIM_TIL_AUTO_GRASS_15, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_15 }},
        { GFX_ANIM_TIL_AUTO_GRASS_16, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_16 }},
        { GFX_ANIM_TIL_AUTO_GRASS_17, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_17 }},
        { GFX_ANIM_TIL_AUTO_GRASS_18, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_18 }},
        { GFX_ANIM_TIL_AUTO_GRASS_19, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_19 }},
        { GFX_ANIM_TIL_AUTO_GRASS_20, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_20 }},
        { GFX_ANIM_TIL_AUTO_GRASS_21, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_21 }},
        { GFX_ANIM_TIL_AUTO_GRASS_22, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_22 }},
        { GFX_ANIM_TIL_AUTO_GRASS_23, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_23 }},
        { GFX_ANIM_TIL_AUTO_GRASS_24, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_24 }},
        { GFX_ANIM_TIL_AUTO_GRASS_25, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_25 }},
        { GFX_ANIM_TIL_AUTO_GRASS_26, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_26 }},
        { GFX_ANIM_TIL_AUTO_GRASS_27, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_27 }},
        { GFX_ANIM_TIL_AUTO_GRASS_28, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_28 }},
        { GFX_ANIM_TIL_AUTO_GRASS_29, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_29 }},
        { GFX_ANIM_TIL_AUTO_GRASS_30, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_30 }},
        { GFX_ANIM_TIL_AUTO_GRASS_31, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_31 }},
        { GFX_ANIM_TIL_AUTO_GRASS_32, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_32 }},
        { GFX_ANIM_TIL_AUTO_GRASS_33, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_33 }},
        { GFX_ANIM_TIL_AUTO_GRASS_34, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_34 }},
        { GFX_ANIM_TIL_AUTO_GRASS_35, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_35 }},
        { GFX_ANIM_TIL_AUTO_GRASS_36, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_36 }},
        { GFX_ANIM_TIL_AUTO_GRASS_37, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_37 }},
        { GFX_ANIM_TIL_AUTO_GRASS_38, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_38 }},
        { GFX_ANIM_TIL_AUTO_GRASS_39, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_39 }},
        { GFX_ANIM_TIL_AUTO_GRASS_40, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_40 }},
        { GFX_ANIM_TIL_AUTO_GRASS_41, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_41 }},
        { GFX_ANIM_TIL_AUTO_GRASS_42, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_42 }},
        { GFX_ANIM_TIL_AUTO_GRASS_43, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_43 }},
        { GFX_ANIM_TIL_AUTO_GRASS_44, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_44 }},
        { GFX_ANIM_TIL_AUTO_GRASS_45, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_45 }},
        { GFX_ANIM_TIL_AUTO_GRASS_46, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_46 }},
        { GFX_ANIM_TIL_AUTO_GRASS_47, SFX_FILE_NONE,          60,        1,        0, { GFX_FRAME_TIL_AUTO_GRASS_47 }},
    };

    Material materials[] = {
        { MATERIAL_NONE },
        // id             footstep sound
        { MATERIAL_GRASS, SFX_FILE_FOOTSTEP_GRASS },
        { MATERIAL_STONE, SFX_FILE_FOOTSTEP_STONE },
        { MATERIAL_WATER, SFX_FILE_FOOTSTEP_GRASS },
    };

    TileType tileTypes[] = {
        { TILE_NONE },
        //id               gfx/anim                 material        auto_mask   flags
        { TILE_GRASS,      GFX_ANIM_TIL_GRASS,      MATERIAL_GRASS, 0b00000000, TILE_FLAG_WALK },
        { TILE_STONE_PATH, GFX_ANIM_TIL_STONE_PATH, MATERIAL_STONE, 0b00000000, TILE_FLAG_WALK },
        { TILE_WATER,      GFX_ANIM_TIL_WATER,      MATERIAL_WATER, 0b00000000, TILE_FLAG_SWIM },
        { TILE_WALL,       GFX_ANIM_TIL_WALL,       MATERIAL_STONE, 0b00000000, TILE_FLAG_COLLIDE },

        { TILE_AUTO_GRASS_00, GFX_ANIM_TIL_AUTO_GRASS_00, MATERIAL_GRASS, 0b00000010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_01, GFX_ANIM_TIL_AUTO_GRASS_01, MATERIAL_GRASS, 0b01000010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_02, GFX_ANIM_TIL_AUTO_GRASS_02, MATERIAL_GRASS, 0b01000000, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_03, GFX_ANIM_TIL_AUTO_GRASS_03, MATERIAL_GRASS, 0b00000000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_04, GFX_ANIM_TIL_AUTO_GRASS_04, MATERIAL_GRASS, 0b00001010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_05, GFX_ANIM_TIL_AUTO_GRASS_05, MATERIAL_GRASS, 0b01001010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_06, GFX_ANIM_TIL_AUTO_GRASS_06, MATERIAL_GRASS, 0b01001000, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_07, GFX_ANIM_TIL_AUTO_GRASS_07, MATERIAL_GRASS, 0b00001000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_08, GFX_ANIM_TIL_AUTO_GRASS_08, MATERIAL_GRASS, 0b00011010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_09, GFX_ANIM_TIL_AUTO_GRASS_09, MATERIAL_GRASS, 0b01011010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_10, GFX_ANIM_TIL_AUTO_GRASS_10, MATERIAL_GRASS, 0b01011000, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_11, GFX_ANIM_TIL_AUTO_GRASS_11, MATERIAL_GRASS, 0b00011000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_12, GFX_ANIM_TIL_AUTO_GRASS_12, MATERIAL_GRASS, 0b00010010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_13, GFX_ANIM_TIL_AUTO_GRASS_13, MATERIAL_GRASS, 0b01010010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_14, GFX_ANIM_TIL_AUTO_GRASS_14, MATERIAL_GRASS, 0b01010000, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_15, GFX_ANIM_TIL_AUTO_GRASS_15, MATERIAL_GRASS, 0b00010000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_16, GFX_ANIM_TIL_AUTO_GRASS_16, MATERIAL_GRASS, 0b11011010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_17, GFX_ANIM_TIL_AUTO_GRASS_17, MATERIAL_GRASS, 0b01001011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_18, GFX_ANIM_TIL_AUTO_GRASS_18, MATERIAL_GRASS, 0b01101010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_19, GFX_ANIM_TIL_AUTO_GRASS_19, MATERIAL_GRASS, 0b01011110, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_20, GFX_ANIM_TIL_AUTO_GRASS_20, MATERIAL_GRASS, 0b00011011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_21, GFX_ANIM_TIL_AUTO_GRASS_21, MATERIAL_GRASS, 0b01111111, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_22, GFX_ANIM_TIL_AUTO_GRASS_22, MATERIAL_GRASS, 0b11111011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_23, GFX_ANIM_TIL_AUTO_GRASS_23, MATERIAL_GRASS, 0b01111000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_24, GFX_ANIM_TIL_AUTO_GRASS_24, MATERIAL_GRASS, 0b00011110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_25, GFX_ANIM_TIL_AUTO_GRASS_25, MATERIAL_GRASS, 0b11011111, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_26, GFX_ANIM_TIL_AUTO_GRASS_26, MATERIAL_GRASS, 0b11111110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_27, GFX_ANIM_TIL_AUTO_GRASS_27, MATERIAL_GRASS, 0b11011000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_28, GFX_ANIM_TIL_AUTO_GRASS_28, MATERIAL_GRASS, 0b01111010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_29, GFX_ANIM_TIL_AUTO_GRASS_29, MATERIAL_GRASS, 0b01010110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_30, GFX_ANIM_TIL_AUTO_GRASS_30, MATERIAL_GRASS, 0b11010010, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_31, GFX_ANIM_TIL_AUTO_GRASS_31, MATERIAL_GRASS, 0b01011011, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_32, GFX_ANIM_TIL_AUTO_GRASS_32, MATERIAL_GRASS, 0b00001011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_33, GFX_ANIM_TIL_AUTO_GRASS_33, MATERIAL_GRASS, 0b01101011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_34, GFX_ANIM_TIL_AUTO_GRASS_34, MATERIAL_GRASS, 0b01111011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_35, GFX_ANIM_TIL_AUTO_GRASS_35, MATERIAL_GRASS, 0b01101000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_36, GFX_ANIM_TIL_AUTO_GRASS_36, MATERIAL_GRASS, 0b01011111, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_37, GFX_ANIM_TIL_AUTO_GRASS_37, MATERIAL_GRASS, 0b01111110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_38, GFX_ANIM_TIL_AUTO_GRASS_38, MATERIAL_GRASS, 0b11111111, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_39, GFX_ANIM_TIL_AUTO_GRASS_39, MATERIAL_GRASS, 0b11111000, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_40, GFX_ANIM_TIL_AUTO_GRASS_40, MATERIAL_GRASS, 0b00011111, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_41, GFX_ANIM_TIL_AUTO_GRASS_41, MATERIAL_GRASS, 0b00000000, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_42, GFX_ANIM_TIL_AUTO_GRASS_42, MATERIAL_GRASS, 0b11011011, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_43, GFX_ANIM_TIL_AUTO_GRASS_43, MATERIAL_GRASS, 0b11111010, TILE_FLAG_WALK },

        { TILE_AUTO_GRASS_44, GFX_ANIM_TIL_AUTO_GRASS_44, MATERIAL_GRASS, 0b00010110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_45, GFX_ANIM_TIL_AUTO_GRASS_45, MATERIAL_GRASS, 0b11011110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_46, GFX_ANIM_TIL_AUTO_GRASS_46, MATERIAL_GRASS, 0b11010110, TILE_FLAG_WALK },
        { TILE_AUTO_GRASS_47, GFX_ANIM_TIL_AUTO_GRASS_47, MATERIAL_GRASS, 0b11010000, TILE_FLAG_WALK },
    };

    void Init(void)
    {
        // Ensure every array element is initialized and in contiguous order by id
        #define ID_CHECK(type, name, arr) \
            for (type name : arr) { \
                static int i = 0; \
                assert(name.id == i++); \
            }

        ID_CHECK(GfxFile  &, gfxFile,  gfxFiles);
        ID_CHECK(MusFile  &, musFile,  musFiles);
        ID_CHECK(SfxFile  &, sfxFile,  sfxFiles);
        ID_CHECK(GfxFrame &, gfxFrame, gfxFrames);
        ID_CHECK(GfxAnim  &, gfxAnim,  gfxAnims);
        ID_CHECK(Material &, material, materials);
        ID_CHECK(TileType &, tileType, tileTypes);
        #undef ID_CHECK

        for (GfxFile &gfxFile : gfxFiles) {
            if (!gfxFile.path) continue;
            gfxFile.texture = LoadTexture(gfxFile.path);
        }
        for (MusFile &musFile : musFiles) {
            if (!musFile.path) continue;
            musFile.music = LoadMusicStream(musFile.path);
        }
        for (SfxFile &sfxFile : sfxFiles) {
            if (!sfxFile.path) continue;
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

        Save("dat/test.dat");
    }
    void Free(void)
    {
        for (GfxFile &gfxFile : gfxFiles) {
            UnloadTexture(gfxFile.texture);
        }
        for (MusFile &musFile : musFiles) {
            UnloadMusicStream(musFile.music);
        }
        for (SfxFile &sfxFile : sfxFiles) {
            UnloadSound(sfxFile.sound);
        }
    }

    void WriteString(FILE *f, const char *str)
    {
        uint16_t len = str ? (uint16_t)strlen(str) : 0;
        fwrite(&len, sizeof(len), 1, f);
        if (len) {
            fwrite(str, sizeof(*str), len, f);
        }
        fflush(f);
    }

    Err Save(const char *filename)
    {
#define WRITE(v) { const auto vv = v; fwrite(&vv, sizeof(vv), 1, f); fflush(f); }

        Err err = RN_SUCCESS;
        FILE *f = fopen(filename, "wb");
        do {
            if (!f) {
                err = RN_BAD_FILE_WRITE; break;
            }

            const char magic[]{ (char)0xDB, (char)0xBB, (char)0x91, (char)0x92 };
            int version = 1;

            // magic
            WRITE(magic[0]);
            WRITE(magic[1]);
            WRITE(magic[2]);
            WRITE(magic[3]);

            // version
            WRITE(version);

            // https://formats.kaitai.io/doom_wad/doom_wad.svg
            // GFXFILE
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(gfxFiles));
            WRITE(DAT_TYP_GFX_FILE);
            for (GfxFile &gfxFile : gfxFiles) {
                WRITE(gfxFile.id);
                WriteString(f, gfxFile.path);
            }

            // MUSFILE
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(musFiles));
            WRITE(DAT_TYP_MUS_FILE);
            for (MusFile &musFile : musFiles) {
                WRITE(musFile.id);
                WriteString(f, musFile.path);
            }

            // SFXFILE
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(sfxFiles));
            WRITE(DAT_TYP_SFX_FILE);
            for (SfxFile &sfxFile : sfxFiles) {
                WRITE(sfxFile.id);
                WriteString(f, sfxFile.path);
                WRITE(sfxFile.pitch_variance);
            }

            // GFXFRAME
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(gfxFrames));
            WRITE(DAT_TYP_GFX_FRAME);
            for (GfxFrame &gfxFrame : gfxFrames) {
                WRITE(gfxFrame.id);
                WRITE(gfxFrame.gfx);
                WRITE(gfxFrame.x);
                WRITE(gfxFrame.y);
                WRITE(gfxFrame.w);
                WRITE(gfxFrame.h);
            }

            // GFXANIM
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(gfxAnims));
            WRITE(DAT_TYP_GFX_ANIM);
            for (GfxAnim &gfxAnim : gfxAnims) {
                WRITE(gfxAnim.id);
                WRITE(gfxAnim.sound);
                WRITE(gfxAnim.frameRate);
                WRITE(gfxAnim.frameCount);
                WRITE(gfxAnim.frameDelay);
                for (int i = 0; i < gfxAnim.frameCount; i++) {
                    WRITE(gfxAnim.frames[i]);
                }
            }

            // MATERIAL
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(materials));
            WRITE(DAT_TYP_MATERIAL);
            for (Material &material : materials) {
                WRITE(material.id);
                WRITE(material.footstepSnd);
            }

            // TILETYPE
            WRITE(DAT_TYP_ARRAY);
            WRITE((int)ARRAY_SIZE(tileTypes));
            WRITE(DAT_TYP_TILE_TYPE);
            for (TileType &tileType : tileTypes) {
                WRITE(tileType.id);
                WRITE(tileType.anim);
                WRITE(tileType.material);
                WRITE(tileType.flags);
                WRITE(tileType.autoTileMask);
            }
        } while (0);

        if (f) {
            fclose(f);
        }
        return err;
#undef WRITE
    }

    Err Load(const char *filename)
    {
        //extern GfxFile gfxFiles[];
        //extern MusFile musFiles[];
        //extern SfxFile sfxFiles[];
        //extern GfxFrame gfxFrames[];
        //extern GfxAnim gfxAnims[];
        //extern TileMat tileMats[];
        //extern TileType tileTypes[];
        return RN_SUCCESS;
    }

    void PlaySound(SfxFileId id, bool multi, float pitchVariance)
    {
        SfxFile &sfxFile = sfxFiles[id];
        float variance = pitchVariance ? pitchVariance : sfxFile.pitch_variance;
        SetSoundPitch(sfxFile.sound, 1.0f + GetRandomFloatVariance(variance));

        if (multi) {
            PlaySoundMulti(sfxFile.sound);
        } else if (!IsSoundPlaying(sfxFile.sound)) {
            PlaySound(sfxFile.sound);
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
        //pos.x = floor(pos.x);
        //pos.y = floor(pos.y);
        DrawTextureRec(file.texture, frameRect, pos, WHITE);
    }
}