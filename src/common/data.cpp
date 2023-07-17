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
    ENUM_STR_GENERATOR(EntityType, ENTITY_TYPES);

#undef ENUM_STR_GENERATOR

    Pack pack1{};

    GfxFile gfxFiles[] = {
        { GFX_FILE_NONE },
        // id                      texture path
        { GFX_FILE_DLG_NPATCH,     "resources/npatch.png" },
        { GFX_FILE_CHR_MAGE,       "resources/mage.png" },
        { GFX_FILE_NPC_LILY,       "resources/lily.png" },
        { GFX_FILE_OBJ_CAMPFIRE,   "resources/campfire.png" },
        { GFX_FILE_PRJ_FIREBALL,   "resources/fireball.png" },
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
        // id                      sound file path                 pitch variance  multi
        { SFX_FILE_SOFT_TICK,      "resources/soft_tick.wav"     , 0.03f,          true },
        { SFX_FILE_CAMPFIRE,       "resources/campfire.wav"      , 0.00f,          false },
        { SFX_FILE_FOOTSTEP_GRASS, "resources/footstep_grass.wav", 0.00f,          true },
        { SFX_FILE_FOOTSTEP_STONE, "resources/footstep_stone.wav", 0.00f,          true },
        { SFX_FILE_FIREBALL,       "resources/fireball.wav",       0.10f,          true },
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
        { GFX_FRAME_PRJ_FIREBALL_0, GFX_FILE_PRJ_FIREBALL,    0,  0,   8,   8 },

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
        { GFX_ANIM_PRJ_FIREBALL,      SFX_FILE_FIREBALL,      60,        1,        0, { GFX_FRAME_PRJ_FIREBALL_0 }},

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
            if (!gfxFile.path.size()) continue;
            gfxFile.texture = LoadTexture(gfxFile.path.c_str());
        }
        for (MusFile &musFile : musFiles) {
            if (!musFile.path.size()) continue;
            musFile.music = LoadMusicStream(musFile.path.c_str());
        }
        for (SfxFile &sfxFile : sfxFiles) {
            if (!sfxFile.path.size()) continue;
            sfxFile.sound = LoadSound(sfxFile.path.c_str());
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

        Err err;
        err = Save("dat/test.dat");
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to save data file.\n");
        }
        err = Load("dat/test.dat");
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load data file.\n");
        }
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

#define PROC(v) stream.process(&v, sizeof(v), 1, stream.f);

    void Process(PackStream &stream, std::string &str)
    {
        uint16_t len = (uint16_t)str.size();
        PROC(len);
        str.resize(len);
        stream.process(str.data(), 1, len, stream.f);
    }

    void Process(PackStream &stream, GfxFile &gfxFile)
    {
        PROC(gfxFile.id);
        Process(stream, gfxFile.path);
    }

    void Process(PackStream &stream, MusFile &musFile)
    {
        PROC(musFile.id);
        Process(stream, musFile.path);
    }

    void Process(PackStream &stream, SfxFile &sfxFile)
    {
        PROC(sfxFile.id);
        Process(stream, sfxFile.path);
    }

    void Process(PackStream &stream, GfxFrame &gfxFrame)
    {
        PROC(gfxFrame.id);
        PROC(gfxFrame.gfx);
        PROC(gfxFrame.x);
        PROC(gfxFrame.y);
        PROC(gfxFrame.w);
        PROC(gfxFrame.h);
    }

    void Process(PackStream &stream, GfxAnim &gfxAnim)
    {
        PROC(gfxAnim.id);
        PROC(gfxAnim.sound);
        PROC(gfxAnim.frameRate);
        PROC(gfxAnim.frameCount);
        PROC(gfxAnim.frameDelay);
        for (int i = 0; i < gfxAnim.frameCount; i++) {
            PROC(gfxAnim.frames[i]);
        }
    }

    void Process(PackStream &stream, Material &material)
    {
        PROC(material.id);
        PROC(material.footstepSnd);
    }

    void Process(PackStream &stream, TileType &tileType)
    {
        PROC(tileType.id);
        PROC(tileType.anim);
        PROC(tileType.material);
        PROC(tileType.flags);
        PROC(tileType.autoTileMask);
    }

    void Process(PackStream &stream, AspectCombat &combat) {
    }

    void Process(PackStream &stream, AspectCollision &collision) {
    }

    void Process(PackStream &stream, AspectDialog &dialog) {
    }

    void Process(PackStream &stream, AspectLife &life) {
    }

    void Process(PackStream &stream, AspectPathfind &pathfind) {
    }

    void Process(PackStream &stream, AspectPhysics &physics) {
    }

    void Process(PackStream &stream, Sprite &sprite) {
    }

    void Process(PackStream &stream, AspectWarp &warp)
    {
        PROC(warp.collider.x);
        PROC(warp.collider.y);
        PROC(warp.collider.width);
        PROC(warp.collider.height);

        PROC(warp.destPos.x);
        PROC(warp.destPos.y);

        Process(stream, warp.destMap);
        Process(stream, warp.templateMap);
        Process(stream, warp.templateTileset);

        //Vector2 TL{ 1632, 404 };
        //Vector2 BR{ 1696, 416 };
        //Vector2 size = Vector2Subtract(BR, TL);
        //Rectangle warpRect{};
        //warpRect.x = TL.x;
        //warpRect.y = TL.y;
        //warpRect.width = size.x;
        //warpRect.height = size.y;

        //warp.collider = warpRect;
        //// Bottom center of warp (assume maps line up and are same size for now)
        //warp.destPos.x = BR.x - size.x / 2;
        //warp.destPos.y = BR.y;
        //warp.templateMap = "maps/cave.dat";
        //warp.templateTileset = "resources/wang/tileset2x2.png";
    }

    void Process(PackStream &stream, Entity &entity)
    {
        bool alive = entity.id && !entity.despawnedAt && entity.type;
        PROC(alive);
        if (alive) {
            PROC(entity.id);
            PROC(entity.mapId);
            PROC(entity.type);
            PROC(entity.spawnedAt);
            PROC(entity.position.x);
            PROC(entity.position.y);
        }

#define PROCESS_ARRAY(arr) \
        for (auto &i : stream.pack->arr) { Process(stream, i); }

        PROCESS_ARRAY(combat   );
        PROCESS_ARRAY(collision);
        PROCESS_ARRAY(dialog   );
        PROCESS_ARRAY(life     );
        PROCESS_ARRAY(pathfind );
        PROCESS_ARRAY(physics  );
        PROCESS_ARRAY(sprite   );
        PROCESS_ARRAY(warp     );

#undef PROCESS_ARRAY
    }

    Err Process(PackStream &stream)
    {
        static const int MAGIC = 0x9291BBDB;
        static const int VERSION = 1;

        Err err = RN_SUCCESS;

        Pack &pack = *stream.pack;

        pack.magic = MAGIC;
        PROC(pack.magic);
        if (pack.magic != MAGIC) {
            return RN_BAD_FILE_READ;
        }

        pack.version = VERSION;
        PROC(pack.version);
        if (pack.version > VERSION) {
            return RN_BAD_FILE_READ;
        }

        int tocOffsetPos = ftell(stream.f);
        pack.tocOffset = 0;
        PROC(pack.tocOffset);

        if (stream.process == (ProcessFn)fwrite) {

#define WRITE_ARRAY(arr) \
    for (auto &i : arr) { \
        pack.toc.entries.push_back(PackTocEntry(i.dtype, ftell(stream.f))); \
        Process(stream, i); \
    }

            // TODO: These should all be pack.blah
            WRITE_ARRAY(gfxFiles);
            WRITE_ARRAY(musFiles);
            WRITE_ARRAY(sfxFiles);
            WRITE_ARRAY(gfxFrames);
            WRITE_ARRAY(gfxAnims);
            WRITE_ARRAY(materials);
            WRITE_ARRAY(tileTypes);
            WRITE_ARRAY(pack.entities);

#undef WRITE_ARRAY

            int tocOffset = ftell(stream.f);
            int entryCount = (int)pack.toc.entries.size();
            PROC(entryCount);
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
            }

            fseek(stream.f, tocOffsetPos, SEEK_SET);
            PROC(tocOffset);
        } else {
            fseek(stream.f, pack.tocOffset, SEEK_SET);
            int entryCount = 0;
            PROC(entryCount);
            pack.toc.entries.resize(entryCount);

            int typeCounts[DAT_TYP_COUNT]{};
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
                typeCounts[tocEntry.dtype]++;
            }

            pack.gfxFiles.resize(typeCounts[DAT_TYP_GFX_FILE]);
            pack.musFiles.resize(typeCounts[DAT_TYP_MUS_FILE]);
            pack.sfxFiles.resize(typeCounts[DAT_TYP_SFX_FILE]);
            pack.gfxFrames.resize(typeCounts[DAT_TYP_GFX_FRAME]);
            pack.gfxAnims.resize(typeCounts[DAT_TYP_GFX_ANIM]);
            pack.materials.resize(typeCounts[DAT_TYP_MATERIAL]);
            pack.tileTypes.resize(typeCounts[DAT_TYP_TILE_TYPE]);

            assert(typeCounts[DAT_TYP_ENTITY] == SV_MAX_ENTITIES);
            pack.entities .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.combat   .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.collision.resize(typeCounts[DAT_TYP_ENTITY]);
            pack.dialog   .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.life     .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.pathfind .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.physics  .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.sprite   .resize(typeCounts[DAT_TYP_ENTITY]);
            pack.warp     .resize(typeCounts[DAT_TYP_ENTITY]);

            int typeNextIndex[DAT_TYP_COUNT]{};

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.f, tocEntry.offset, SEEK_SET);
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  Process(stream, pack.gfxFiles [typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_MUS_FILE:  Process(stream, pack.musFiles [typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_SFX_FILE:  Process(stream, pack.sfxFiles [typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_GFX_FRAME: Process(stream, pack.gfxFrames[typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_GFX_ANIM:  Process(stream, pack.gfxAnims [typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_MATERIAL:  Process(stream, pack.materials[typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_TILE_TYPE: Process(stream, pack.tileTypes[typeNextIndex[tocEntry.dtype]++]); break;
                    case DAT_TYP_ENTITY:    Process(stream, pack.entities [typeNextIndex[tocEntry.dtype]++]); break;
                }
            }
        }

        return err;
    }
#undef PROC

    Err Save(const char *filename)
    {
        Err err = RN_SUCCESS;

        PackStream stream{};
        stream.f = fopen(filename, "wb");
        if (!stream.f) {
            return RN_BAD_FILE_WRITE;
        }

        stream.process = (ProcessFn)fwrite;
        stream.pack = &pack1;

        err = Process(stream);

        fclose(stream.f);
        return err;
    }

    Err Validate(Pack &pack)
    {
        Err err = RN_SUCCESS;

        // Ensure every array element is initialized and in contiguous order by id
#define ID_CHECK(type, name, arr)                       \
            for (type name : arr) {                     \
                static int i = 0;                       \
                if (name.id != i) {                     \
                    assert(!"expected contiguous IDs"); \
                    return RN_BAD_FILE_READ;            \
                }                                       \
                i++;                                    \
            }

        ID_CHECK(GfxFile  &, gfxFile,  pack.gfxFiles);
        ID_CHECK(MusFile  &, musFile,  pack.musFiles);
        ID_CHECK(SfxFile  &, sfxFile,  pack.sfxFiles);
        ID_CHECK(GfxFrame &, gfxFrame, pack.gfxFrames);
        ID_CHECK(GfxAnim  &, gfxAnim,  pack.gfxAnims);
        ID_CHECK(Material &, material, pack.materials);
        ID_CHECK(TileType &, tileType, pack.tileTypes);
#undef ID_CHECK

        for (GfxFile &gfxFile : pack.gfxFiles) {
            if (!gfxFile.path.size()) continue;
            gfxFile.texture = LoadTexture(gfxFile.path.c_str());
        }
        for (MusFile &musFile : pack.musFiles) {
            if (!musFile.path.size()) continue;
            musFile.music = LoadMusicStream(musFile.path.c_str());
        }
        for (SfxFile &sfxFile : pack.sfxFiles) {
            if (!sfxFile.path.size()) continue;
            sfxFile.sound = LoadSound(sfxFile.path.c_str());
        }

        // Generate checkerboard image in slot 0 as a placeholder for when other images fail to load
        Image placeholderImg = GenImageChecked(16, 16, 4, 4, MAGENTA, WHITE);
        if (placeholderImg.width) {
            Texture placeholderTex = LoadTextureFromImage(placeholderImg);
            if (placeholderTex.width) {
                pack.gfxFiles[0].texture = placeholderTex;
                pack.gfxFrames[0].gfx = GFX_FILE_NONE;
                pack.gfxFrames[0].w = placeholderTex.width;
                pack.gfxFrames[0].h = placeholderTex.height;
            } else {
                printf("[data] WARN: Failed to generate placeholder texture\n");
            }
            UnloadImage(placeholderImg);
        } else {
            printf("[data] WARN: Failed to generate placeholder image\n");
        }

        return err;
    }

    Err Load(const char *filename)
    {
        Err err = RN_SUCCESS;

        PackStream stream{};
        stream.f = fopen(filename, "rb");
        if (!stream.f) {
            return RN_BAD_FILE_READ;
        }
        stream.process = (ProcessFn)fread;
        stream.pack = &pack1;
        err = Process(stream);
        fclose(stream.f);

        if (!err) {
            err = Validate(*stream.pack);
        }

        return err;
    }

    void PlaySound(SfxFileId id, float pitchVariance)
    {
        SfxFile &sfxFile = sfxFiles[id];
        float variance = pitchVariance ? pitchVariance : sfxFile.pitch_variance;
        SetSoundPitch(sfxFile.sound, 1.0f + GetRandomFloatVariance(variance));

        if (sfxFile.multi) {
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
    void UpdateSprite(Sprite &sprite, EntityType entityType, Vector2 velocity, double dt, bool newlySpawned)
    {
        sprite.animAccum += dt;

        // TODO: Make this more general and stop taking in entityType.
        switch (entityType) {
            case ENTITY_PLAYER: case ENTITY_NPC: {
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

        if (newlySpawned && anim.sound) {
            PlaySound(anim.sound);
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