#include "data.h"
#include <sstream>
#include <iomanip>

namespace data {
    Err LoadResources(Pack &pack);

#define ENUM_STR_GENERATOR(type, enumDef, enumGen) \
    const char *type##Str(type id) {               \
        switch (id) {                              \
            enumDef(enumGen)                       \
            default: return "<unknown>";           \
        }                                          \
    }

    ENUM_STR_GENERATOR(DataType,   DATA_TYPES   , ENUM_GEN_CASE_RETURN_DESC);
    ENUM_STR_GENERATOR(GfxFileId,  GFX_FILE_IDS , ENUM_GEN_CASE_RETURN_STR);
    //ENUM_STR_GENERATOR(MusFileId,  MUS_FILE_IDS , ENUM_GEN_CASE_RETURN_STR);
    //ENUM_STR_GENERATOR(SfxFileId,  SFX_FILE_IDS , ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(GfxFrameId, GFX_FRAME_IDS, ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(GfxAnimId,  GFX_ANIM_IDS , ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(MaterialId, MATERIAL_IDS , ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(SpriteId,   SPRITE_IDS,    ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(TileTypeId, TILE_TYPE_IDS, ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(EntityType, ENTITY_TYPES , ENUM_GEN_CASE_RETURN_STR);

#undef ENUM_STR_GENERATOR

    std::vector<Pack *> packs{};

    struct GameState {
        bool freye_introduced;
    };
    GameState game_state{};

    uint32_t FreyeIntroListener(uint32_t source_id, uint32_t target_id, uint32_t dialog_id) {
        uint32_t final_dialog_id = dialog_id;
        if (!game_state.freye_introduced) {
            // Freye not yet introduced, redirect player to full introduction
            Dialog *first_time_intro = dialog_library.FindByKey("DIALOG_FREYE_INTRO_FIRST_TIME");
            if (first_time_intro) {
                final_dialog_id = first_time_intro->id;
            } else {
                assert(!"probably this shouldn't fail bro, fix ur dialogs");
            }
            game_state.freye_introduced = true;
        }
        return final_dialog_id;
    }

    void ReadFileIntoDataBuffer(std::string filename, DatBuffer &datBuffer)
    {
        uint32_t bytes_read = 0;
        datBuffer.bytes = LoadFileData(filename.c_str(), &bytes_read);
        datBuffer.length = bytes_read;
    }

    void FreeDataBuffer(DatBuffer &datBuffer)
    {
        if (datBuffer.length) {
            MemFree(datBuffer.bytes);
            datBuffer = {};
        }
    }

#if 0
#define LINE_MAX 1024
#define KEY_MAX  64
#define PATH_MAX 512
    Err LoadMusicIndexOrig(std::string path, std::vector<MusFile> &mus_files)
    {
        double start = GetTime();
        Err err = RN_SUCCESS;

        char *buf = LoadFileText(path.c_str());
        printf("C loaded file text in %f seconds\n", GetTime() - start);

        if (buf) {
            char buf_line[LINE_MAX + 1];
            char buf_key[KEY_MAX + 1];
            char buf_path[PATH_MAX + 1];

            char *cur = buf;
            int bytes_read = 0;

            while (sscanf(cur, "%" STRSTR(LINE_MAX) "[^\n]\n%n", buf_line, &bytes_read) == 1) {
                cur += bytes_read;
                bytes_read = 0;

                char closing_quote = 0;
                int arg_count = sscanf(buf_line,
                    "%" STRSTR(KEY_MAX) "[A-Za-z0-9_] \"%" STRSTR(PATH_MAX) "[^\"]%c",
                    buf_key, buf_path, &closing_quote);

                if (arg_count != 3 || closing_quote != '\"') {
                    continue;
                }

                MusFile mus_file{};
                mus_file.id = buf_key;
                mus_file.path = buf_path;
                mus_files.push_back(mus_file);
            }

            UnloadFileText(buf);
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        printf("C loaded %zu records in %f seconds\n", mus_files.size(), GetTime() - start);
        return err;
    }
#undef LINE_MAX
#undef KEY_MAX
#undef PATH_MAX
#endif

    Err LoadMusicIndex(std::string path, std::vector<MusFile> &mus_files)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;

            while (std::getline(inputFile, line)) {
                std::istringstream iss(line);

                MusFile mus_file{};
                if (iss
                    >> mus_file.id
                    >> std::quoted(mus_file.path)
                    && mus_file.id[0] != '#'
                ) {
                    mus_files.push_back(mus_file);
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }

    Err LoadSoundIndex(std::string path, std::vector<SfxFile> &sfx_files)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;

            while (std::getline(inputFile, line)) {
                std::istringstream iss(line);

                SfxFile sfx_file{};
                if (iss
                    >> sfx_file.id
                    >> std::quoted(sfx_file.path)
                    >> sfx_file.pitch_variance
                    >> std::boolalpha >> sfx_file.multi
                    && sfx_file.id[0] != '#'
                ) {
                    sfx_files.push_back(sfx_file);
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }

    void Init(void)
    {
        Err err = RN_SUCCESS;

#if 1
        GfxFile gfx_files[] = {
            { GFX_FILE_NONE },
            // id                       texture path
            { GFX_FILE_DLG_NPATCH,     "resources/texture/npatch.png" },
            { GFX_FILE_CHR_MAGE,       "resources/texture/mage.png" },
            { GFX_FILE_NPC_LILY,       "resources/texture/lily.png" },
            { GFX_FILE_NPC_FREYE,      "resources/texture/freye.png" },
            { GFX_FILE_NPC_NESSA,      "resources/texture/nessa.png" },
            { GFX_FILE_NPC_ELANE,      "resources/texture/elane.png" },
            { GFX_FILE_OBJ_CAMPFIRE,   "resources/texture/campfire.png" },
            { GFX_FILE_PRJ_FIREBALL,   "resources/texture/fireball.png" },
            { GFX_FILE_TIL_OVERWORLD,  "resources/texture/tiles32.png" },
            { GFX_FILE_TIL_AUTO_GRASS, "resources/texture/autotile_3x3_min.png" },
        };

        //MusFile mus_files[] = {
        //    { "NONE" },
        //    // id                  music file path
        //    { "AMBIENT_OUTDOORS", "resources/music/copyright/345470__philip_goddard__branscombe-landslip-birds-and-sea-echoes-ese-from-cave-track.ogg" },
        //    { "AMBIENT_CAVE",     "resources/music/copyright/69391__zixem__cave_amb.wav" },
        //};

        //SfxFile sfx_files[] = {
        //    { SFX_FILE_NONE },
        //    // id                       sound file path                      pitch variance  multi
        //    { SFX_FILE_SOFT_TICK,      "resources/sound/soft_tick.wav"     , 0.03f,          true },
        //    { SFX_FILE_CAMPFIRE,       "resources/sound/campfire.wav"      , 0.00f,          false },
        //    { SFX_FILE_FOOTSTEP_GRASS, "resources/sound/footstep_grass.wav", 0.00f,          true },
        //    { SFX_FILE_FOOTSTEP_STONE, "resources/sound/footstep_stone.wav", 0.00f,          true },
        //    { SFX_FILE_FIREBALL,       "resources/sound/fireball.wav",       0.05f,          true },
        //};

        GfxFrame gfx_frames[] = {
            // id                       image file                x   y    w    h
            { GFX_FRAME_NONE },

            // playable characters
            { GFX_FRAME_CHR_MAGE_E_0,   GFX_FILE_CHR_MAGE,        0,  0,  32,  64 },
            { GFX_FRAME_CHR_MAGE_W_0,   GFX_FILE_CHR_MAGE,       32,  0,  32,  64 },

            // npcs
            { GFX_FRAME_NPC_LILY_E_0,   GFX_FILE_NPC_LILY,        0,  0,  32,  64 },
            { GFX_FRAME_NPC_LILY_W_0,   GFX_FILE_NPC_LILY,       32,  0,  32,  64 },
            { GFX_FRAME_NPC_FREYE_E_0,  GFX_FILE_NPC_FREYE,       0,  0,  32,  64 },
            { GFX_FRAME_NPC_FREYE_W_0,  GFX_FILE_NPC_FREYE,      32,  0,  32,  64 },
            { GFX_FRAME_NPC_NESSA_E_0,  GFX_FILE_NPC_NESSA,       0,  0,  32,  64 },
            { GFX_FRAME_NPC_NESSA_W_0,  GFX_FILE_NPC_NESSA,      32,  0,  32,  64 },
            { GFX_FRAME_NPC_ELANE_E_0,  GFX_FILE_NPC_ELANE,       0,  0,  32,  64 },
            { GFX_FRAME_NPC_ELANE_W_0,  GFX_FILE_NPC_ELANE,      32,  0,  32,  64 },

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

        GfxAnim gfx_anims[] = {
            // id                       sound effect  frmRate  frmCount  frmDelay  frames
            { GFX_ANIM_NONE },

            // playable characters
            { GFX_ANIM_CHR_MAGE_E,                "", 60, 1,        0, { GFX_FRAME_CHR_MAGE_E_0 }},
            { GFX_ANIM_CHR_MAGE_W,                "", 60, 1,        0, { GFX_FRAME_CHR_MAGE_W_0 }},

            // npcs
            { GFX_ANIM_NPC_LILY_E,                "", 60, 1,        0, { GFX_FRAME_NPC_LILY_E_0 }},
            { GFX_ANIM_NPC_LILY_W,                "", 60, 1,        0, { GFX_FRAME_NPC_LILY_W_0 }},
            { GFX_ANIM_NPC_FREYE_E,               "", 60, 1,        0, { GFX_FRAME_NPC_FREYE_E_0 }},
            { GFX_ANIM_NPC_FREYE_W,               "", 60, 1,        0, { GFX_FRAME_NPC_FREYE_W_0 }},
            { GFX_ANIM_NPC_NESSA_E,               "", 60, 1,        0, { GFX_FRAME_NPC_NESSA_E_0 }},
            { GFX_ANIM_NPC_NESSA_W,               "", 60, 1,        0, { GFX_FRAME_NPC_NESSA_W_0 }},
            { GFX_ANIM_NPC_ELANE_E,               "", 60, 1,        0, { GFX_FRAME_NPC_ELANE_E_0 }},
            { GFX_ANIM_NPC_ELANE_W,               "", 60, 1,        0, { GFX_FRAME_NPC_ELANE_W_0 }},

            // objects
            { GFX_ANIM_OBJ_CAMPFIRE,  "sfx_campfire", 60, 8,        4, { GFX_FRAME_OBJ_CAMPFIRE_0, GFX_FRAME_OBJ_CAMPFIRE_1, GFX_FRAME_OBJ_CAMPFIRE_2, GFX_FRAME_OBJ_CAMPFIRE_3, GFX_FRAME_OBJ_CAMPFIRE_4, GFX_FRAME_OBJ_CAMPFIRE_5, GFX_FRAME_OBJ_CAMPFIRE_6, GFX_FRAME_OBJ_CAMPFIRE_7 }},

            // projectiles
            { GFX_ANIM_PRJ_FIREBALL,  "sfx_fireball", 60, 1,        0, { GFX_FRAME_PRJ_FIREBALL_0 }},

            // tiles
            { GFX_ANIM_TIL_GRASS,                 "", 60, 1,        0, { GFX_FRAME_TIL_GRASS }},
            { GFX_ANIM_TIL_STONE_PATH,            "", 60, 1,        0, { GFX_FRAME_TIL_STONE_PATH }},
            { GFX_ANIM_TIL_WALL,                  "", 60, 1,        0, { GFX_FRAME_TIL_WALL }},
            { GFX_ANIM_TIL_WATER,                 "", 60, 1,        0, { GFX_FRAME_TIL_WATER_0 }},

            { GFX_ANIM_TIL_AUTO_GRASS_00,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_00 }},
            { GFX_ANIM_TIL_AUTO_GRASS_01,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_01 }},
            { GFX_ANIM_TIL_AUTO_GRASS_02,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_02 }},
            { GFX_ANIM_TIL_AUTO_GRASS_03,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_03 }},
            { GFX_ANIM_TIL_AUTO_GRASS_04,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_04 }},
            { GFX_ANIM_TIL_AUTO_GRASS_05,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_05 }},
            { GFX_ANIM_TIL_AUTO_GRASS_06,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_06 }},
            { GFX_ANIM_TIL_AUTO_GRASS_07,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_07 }},
            { GFX_ANIM_TIL_AUTO_GRASS_08,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_08 }},
            { GFX_ANIM_TIL_AUTO_GRASS_09,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_09 }},
            { GFX_ANIM_TIL_AUTO_GRASS_10,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_10 }},
            { GFX_ANIM_TIL_AUTO_GRASS_11,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_11 }},
            { GFX_ANIM_TIL_AUTO_GRASS_12,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_12 }},
            { GFX_ANIM_TIL_AUTO_GRASS_13,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_13 }},
            { GFX_ANIM_TIL_AUTO_GRASS_14,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_14 }},
            { GFX_ANIM_TIL_AUTO_GRASS_15,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_15 }},
            { GFX_ANIM_TIL_AUTO_GRASS_16,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_16 }},
            { GFX_ANIM_TIL_AUTO_GRASS_17,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_17 }},
            { GFX_ANIM_TIL_AUTO_GRASS_18,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_18 }},
            { GFX_ANIM_TIL_AUTO_GRASS_19,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_19 }},
            { GFX_ANIM_TIL_AUTO_GRASS_20,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_20 }},
            { GFX_ANIM_TIL_AUTO_GRASS_21,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_21 }},
            { GFX_ANIM_TIL_AUTO_GRASS_22,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_22 }},
            { GFX_ANIM_TIL_AUTO_GRASS_23,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_23 }},
            { GFX_ANIM_TIL_AUTO_GRASS_24,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_24 }},
            { GFX_ANIM_TIL_AUTO_GRASS_25,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_25 }},
            { GFX_ANIM_TIL_AUTO_GRASS_26,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_26 }},
            { GFX_ANIM_TIL_AUTO_GRASS_27,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_27 }},
            { GFX_ANIM_TIL_AUTO_GRASS_28,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_28 }},
            { GFX_ANIM_TIL_AUTO_GRASS_29,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_29 }},
            { GFX_ANIM_TIL_AUTO_GRASS_30,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_30 }},
            { GFX_ANIM_TIL_AUTO_GRASS_31,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_31 }},
            { GFX_ANIM_TIL_AUTO_GRASS_32,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_32 }},
            { GFX_ANIM_TIL_AUTO_GRASS_33,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_33 }},
            { GFX_ANIM_TIL_AUTO_GRASS_34,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_34 }},
            { GFX_ANIM_TIL_AUTO_GRASS_35,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_35 }},
            { GFX_ANIM_TIL_AUTO_GRASS_36,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_36 }},
            { GFX_ANIM_TIL_AUTO_GRASS_37,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_37 }},
            { GFX_ANIM_TIL_AUTO_GRASS_38,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_38 }},
            { GFX_ANIM_TIL_AUTO_GRASS_39,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_39 }},
            { GFX_ANIM_TIL_AUTO_GRASS_40,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_40 }},
            { GFX_ANIM_TIL_AUTO_GRASS_41,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_41 }},
            { GFX_ANIM_TIL_AUTO_GRASS_42,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_42 }},
            { GFX_ANIM_TIL_AUTO_GRASS_43,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_43 }},
            { GFX_ANIM_TIL_AUTO_GRASS_44,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_44 }},
            { GFX_ANIM_TIL_AUTO_GRASS_45,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_45 }},
            { GFX_ANIM_TIL_AUTO_GRASS_46,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_46 }},
            { GFX_ANIM_TIL_AUTO_GRASS_47,         "", 60, 1,        0, { GFX_FRAME_TIL_AUTO_GRASS_47 }},
        };

        Material materials[] = {
            // id              footstep sound
            { MATERIAL_NONE , "sfx_none" },
            { MATERIAL_GRASS, "sfx_footstep_grass" },
            { MATERIAL_STONE, "sfx_footstep_stone" },
            { MATERIAL_WATER, "sfx_footstep_grass" },
        };

        Sprite sprites[] = {
            { SPRITE_NONE },
            // id                    anims
            //                       N                      E                     S              W                     NE             SE             SW             NW
            { SPRITE_CHR_MAGE,     { GFX_ANIM_NONE,         GFX_ANIM_CHR_MAGE_E,  GFX_ANIM_NONE, GFX_ANIM_CHR_MAGE_W,  GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_NPC_LILY,     { GFX_ANIM_NONE,         GFX_ANIM_NPC_LILY_E,  GFX_ANIM_NONE, GFX_ANIM_NPC_LILY_W,  GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_NPC_FREYE,    { GFX_ANIM_NONE,         GFX_ANIM_NPC_FREYE_E, GFX_ANIM_NONE, GFX_ANIM_NPC_FREYE_W, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_NPC_NESSA,    { GFX_ANIM_NONE,         GFX_ANIM_NPC_NESSA_E, GFX_ANIM_NONE, GFX_ANIM_NPC_NESSA_W, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_NPC_ELANE,    { GFX_ANIM_NONE,         GFX_ANIM_NPC_ELANE_E, GFX_ANIM_NONE, GFX_ANIM_NPC_ELANE_W, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_OBJ_CAMPFIRE, { GFX_ANIM_OBJ_CAMPFIRE, GFX_ANIM_NONE,        GFX_ANIM_NONE, GFX_ANIM_NONE,        GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
            { SPRITE_PRJ_FIREBALL, { GFX_ANIM_PRJ_FIREBALL, GFX_ANIM_NONE,        GFX_ANIM_NONE, GFX_ANIM_NONE,        GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE, GFX_ANIM_NONE } },
        };

        TileType tile_types[] = {
            { TILE_NONE },
            // id              gfx/anim                 material        auto_mask   flags
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

        std::vector<MusFile> mus_files{};
        err = LoadMusicIndex("resources/music/index.txt", mus_files);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load music index.\n");
        }

        std::vector<SfxFile> sfx_files{};
        err = LoadSoundIndex("resources/sound/index.txt", sfx_files);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load sound index.\n");
        }

        // Ensure every array element is initialized and in contiguous order by id
        #define ID_CHECK(type, name, arr) \
            for (type name : arr) { \
                static int i = 0; \
                assert(name.id == i++); \
            }

        ID_CHECK(GfxFile  &, gfx_file,  gfx_files);
        //ID_CHECK(MusFile  &, mus_file,   mus_files);
        //ID_CHECK(SfxFile  &, sfx_file,  sfx_files);
        ID_CHECK(GfxFrame &, gfx_frame, gfx_frames);
        ID_CHECK(GfxAnim  &, gfx_anim,  gfx_anims);
        ID_CHECK(Material &, material,  materials);
        ID_CHECK(Sprite   &, sprite,    sprites);
        ID_CHECK(TileType &, tile_type, tile_types);
        #undef ID_CHECK

        Pack packHardcoded{ "pack/pack1.dat" };
        for (auto &i : gfx_files)  packHardcoded.gfx_files.push_back(i);
        for (auto &i : mus_files)  packHardcoded.mus_files.push_back(i);
        for (auto &i : sfx_files)  packHardcoded.sfx_files.push_back(i);
        for (auto &i : gfx_frames) packHardcoded.gfx_frames.push_back(i);
        for (auto &i : gfx_anims)  packHardcoded.gfx_anims.push_back(i);
        for (auto &i : materials)  packHardcoded.materials.push_back(i);
        for (auto &i : sprites)    packHardcoded.sprites.push_back(i);
        for (auto &i : tile_types) packHardcoded.tile_types.push_back(i);

        LoadResources(packHardcoded);

        err = SavePack(packHardcoded);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to save data file.\n");
        }
#endif

        Pack *pack1 = new Pack{ "pack/pack1.dat" };
        packs.push_back(pack1);

        err = LoadPack(*pack1);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load data file.\n");
        }

#if 0
        DatBuffer compressMe{};
        uint32_t bytesRead = 0;
        compressMe.bytes = LoadFileData("pack/pack1.dat", &bytesRead);
        compressMe.length = bytesRead;

        DatBuffer compressed{};
        int32_t compressedSize = 0;
        compressed.bytes = CompressData(compressMe.bytes, compressMe.length, &compressedSize);
        compressed.length = compressedSize;

        SaveFileData("pack/pack1.smol", compressed.bytes, compressed.length);

        MemFree(compressed.bytes);
        MemFree(compressMe.bytes);
#endif

        // TODO: Pack these too
        LoadDialogFile("resources/dialog/lily.md");
        LoadDialogFile("resources/dialog/freye.md");
        LoadDialogFile("resources/dialog/nessa.md");
        LoadDialogFile("resources/dialog/elane.md");

        dialog_library.RegisterListener("DIALOG_FREYE_INTRO", FreyeIntroListener);
    }

    void Free(void)
    {
        // NOTE(dlb): ~Material is crashing for unknown reason. double free or trying to free string constant??
        // NOTE(dlb): delete pack takes *forever*. Who cares. Let OS figure it out.
#ifndef FAST_EXIT_SKIP_FREE
        for (auto pack : packs) {
            UnloadPack(*pack);
            delete pack;
        }
        packs.clear();

        for (auto &fileText : dialog_library.dialog_files) {
            UnloadFileText((char *)fileText);
        }
#endif
    }

#define PROC(v) { static_assert(!std::is_same_v<decltype(v), std::string>); stream.process(&v, sizeof(v), 1, stream.f); }

    void Process(PackStream &stream, std::string &str)
    {
        uint16_t strLen = (uint16_t)str.size();
        PROC(strLen);
        str.resize(strLen);
        stream.process(str.data(), 1, strLen, stream.f);
    }

    void Process(PackStream &stream, DatBuffer &buffer)
    {
        PROC(buffer.length);
        if (buffer.length) {
            if (!buffer.bytes) {
                assert(stream.mode == PACK_MODE_READ);
                buffer.bytes = (uint8_t *)MemAlloc(buffer.length);
            }
            stream.process(buffer.bytes, 1, buffer.length, stream.f);
        }
    }

    void Process(PackStream &stream, GfxFile &gfx_file, int index)
    {
        PROC(gfx_file.id);
        Process(stream, gfx_file.path);
        Process(stream, gfx_file.data_buffer);
        if (stream.mode == PACK_MODE_READ && !gfx_file.data_buffer.length && !gfx_file.path.empty()) {
            ReadFileIntoDataBuffer(gfx_file.path.c_str(), gfx_file.data_buffer);
        }
    }

    void Process(PackStream &stream, MusFile &mus_file, int index)
    {
        Process(stream, mus_file.id);
        Process(stream, mus_file.path);

        // NOTE(dlb): Music is streaming, don't read whole file into memory
        //if (stream.pack->version >= 2) {
        //    Process(stream, mus_file.data_buffer);
        //}
        //if (stream.mode == PACK_MODE_READ && !mus_file.data_buffer.length && !mus_file.path.empty()) {
        //    ReadFileIntoDataBuffer(mus_file.path.c_str(), mus_file.data_buffer);
        //}

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->mus_files_by_id[mus_file.id] = index;
        }
    }

    void Process(PackStream &stream, SfxFile &sfx_file, int index)
    {
        Process(stream, sfx_file.id);
        Process(stream, sfx_file.path);
        Process(stream, sfx_file.data_buffer);
        PROC(sfx_file.pitch_variance);
        PROC(sfx_file.multi);
        if (stream.mode == PACK_MODE_READ && !sfx_file.data_buffer.length && !sfx_file.path.empty()) {
            ReadFileIntoDataBuffer(sfx_file.path.c_str(), sfx_file.data_buffer);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->sfx_files_by_id[sfx_file.id] = index;
        }
    }

    void Process(PackStream &stream, GfxFrame &gfx_frame, int index)
    {
        PROC(gfx_frame.id);
        PROC(gfx_frame.gfx);
        PROC(gfx_frame.x);
        PROC(gfx_frame.y);
        PROC(gfx_frame.w);
        PROC(gfx_frame.h);
    }

    void Process(PackStream &stream, GfxAnim &gfx_anim, int index)
    {
        PROC(gfx_anim.id);
        Process(stream, gfx_anim.sound);
        PROC(gfx_anim.frame_rate);
        PROC(gfx_anim.frame_count);
        PROC(gfx_anim.frame_delay);
        assert(gfx_anim.frame_count <= ARRAY_SIZE(gfx_anim.frames));
        for (int i = 0; i < gfx_anim.frame_count; i++) {
            PROC(gfx_anim.frames[i]);
        }
    }

    void Process(PackStream &stream, Material &material, int index)
    {
        PROC(material.id);
        Process(stream, material.footstep_sound);
    }

    void Process(PackStream &stream, Sprite &sprite, int index) {
        PROC(sprite.id);
        assert(ARRAY_SIZE(sprite.anims) == 8); // if this changes, version must increment
        for (int i = 0; i < 8; i++) {
            PROC(sprite.anims[i]);
        }
    }

    void Process(PackStream &stream, TileType &tile_type, int index)
    {
        PROC(tile_type.id);
        PROC(tile_type.anim);
        PROC(tile_type.material);
        PROC(tile_type.flags);
        PROC(tile_type.auto_tile_mask);
    }

    void Process(PackStream &stream, Entity &entity, int index)
    {
        bool alive = entity.id && !entity.despawned_at && entity.type;
        PROC(alive);
        if (!alive) {
            return;
        }

        PROC(entity.id);
        PROC(entity.map_id);
        PROC(entity.type);
        PROC(entity.spawned_at);
        //PROC(entity.despawned_at);
        PROC(entity.position.x);
        PROC(entity.position.y);

        PROC(entity.radius);
        //PROC(entity.colliding);
        //PROC(entity.on_warp);

        //PROC(entity.last_attacked_at);
        //PROC(entity.attack_cooldown);

        Process(stream, entity.dialog_root_key);
        //PROC(entity.dialog_spawned_at);
        //PROC(entity.dialog_id);
        //PROC(entity.dialog_title);
        //PROC(entity.dialog_message);

        PROC(entity.hp_max);
        PROC(entity.hp);
        //PROC(entity.hp_smooth);

        PROC(entity.path_id);
        if (entity.path_id) {
            PROC(entity.path_node_last_reached);
            PROC(entity.path_node_target);
            //PROC(entity.path_node_arrived_at);
        }

        PROC(entity.drag);
        PROC(entity.speed);
        PROC(entity.force_accum);
        PROC(entity.velocity);

        PROC(entity.sprite);
        PROC(entity.direction);
        //PROC(entity.anim_frame);
        //PROC(entity.anim_accum);

        PROC(entity.warp_collider.x);
        PROC(entity.warp_collider.y);
        PROC(entity.warp_collider.width);
        PROC(entity.warp_collider.height);

        PROC(entity.warp_dest_pos.x);
        PROC(entity.warp_dest_pos.y);

        Process(stream, entity.warp_dest_map);
        Process(stream, entity.warp_template_map);
        Process(stream, entity.warp_template_tileset);

        //---------------------------------------
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
        //---------------------------------------
    }

    Err Process(PackStream &stream)
    {
        static const int MAGIC = 0x9291BBDB;
        // v1: the O.G. pack file
        // v2: add data buffers for gfx/mus/sfx
        // v3: add pitch_variance / multi to sfx
        // v4: add sprite resources
        static const int VERSION = 4;

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
        pack.toc_offset = 0;
        PROC(pack.toc_offset);

        if (stream.mode == PACK_MODE_WRITE) {
            #define WRITE_ARRAY(arr) \
                for (auto &i : arr) { \
                    pack.toc.entries.push_back(PackTocEntry(i.dtype, ftell(stream.f))); \
                    Process(stream, i, 0); \
                }

            // TODO: These should all be pack.blah after we've removed the
            // static data and switched to pack editing
            WRITE_ARRAY(pack.gfx_files);
            WRITE_ARRAY(pack.mus_files);
            WRITE_ARRAY(pack.sfx_files);
            WRITE_ARRAY(pack.gfx_frames);
            WRITE_ARRAY(pack.gfx_anims);
            WRITE_ARRAY(pack.materials);
            WRITE_ARRAY(pack.sprites);
            WRITE_ARRAY(pack.tile_types);
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
            fseek(stream.f, pack.toc_offset, SEEK_SET);
            int entryCount = 0;
            PROC(entryCount);
            pack.toc.entries.resize(entryCount);

            int typeCounts[DAT_TYP_COUNT]{};
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
                typeCounts[tocEntry.dtype]++;
            }

            pack.gfx_files .resize(typeCounts[DAT_TYP_GFX_FILE]);
            pack.mus_files .resize(typeCounts[DAT_TYP_MUS_FILE]);
            pack.sfx_files .resize(typeCounts[DAT_TYP_SFX_FILE]);
            pack.gfx_frames.resize(typeCounts[DAT_TYP_GFX_FRAME]);
            pack.gfx_anims .resize(typeCounts[DAT_TYP_GFX_ANIM]);
            pack.materials .resize(typeCounts[DAT_TYP_MATERIAL]);
            pack.sprites   .resize(typeCounts[DAT_TYP_SPRITE]);
            pack.tile_types.resize(typeCounts[DAT_TYP_TILE_TYPE]);

            assert(typeCounts[DAT_TYP_ENTITY] == SV_MAX_ENTITIES);
            pack.entities.resize(typeCounts[DAT_TYP_ENTITY]);

            int typeNextIndex[DAT_TYP_COUNT]{};

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.f, tocEntry.offset, SEEK_SET);
                int &nextIndex = typeNextIndex[tocEntry.dtype];
                tocEntry.index = nextIndex;
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  Process(stream, pack.gfx_files [nextIndex], nextIndex); break;
                    case DAT_TYP_MUS_FILE:  Process(stream, pack.mus_files [nextIndex], nextIndex); break;
                    case DAT_TYP_SFX_FILE:  Process(stream, pack.sfx_files [nextIndex], nextIndex); break;
                    case DAT_TYP_GFX_FRAME: Process(stream, pack.gfx_frames[nextIndex], nextIndex); break;
                    case DAT_TYP_GFX_ANIM:  Process(stream, pack.gfx_anims [nextIndex], nextIndex); break;
                    case DAT_TYP_MATERIAL:  Process(stream, pack.materials [nextIndex], nextIndex); break;
                    case DAT_TYP_SPRITE:    Process(stream, pack.sprites   [nextIndex], nextIndex); break;
                    case DAT_TYP_TILE_TYPE: Process(stream, pack.tile_types[nextIndex], nextIndex); break;
                    case DAT_TYP_ENTITY:    Process(stream, pack.entities  [nextIndex], nextIndex); break;
                }
                nextIndex++;
            }
        }

        return err;
    }
#undef PROC

    Err SavePack(Pack &pack)
    {
        Err err = RN_SUCCESS;

        PackStream stream{};
        stream.mode = PACK_MODE_WRITE;
        stream.f = fopen(pack.path.c_str(), "wb");
        if (!stream.f) {
            return RN_BAD_FILE_WRITE;
        }

        stream.process = (ProcessFn)fwrite;
        stream.pack = &pack;

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

        ID_CHECK(GfxFile  &, gfx_file,  pack.gfx_files);
        //ID_CHECK(MusFile  &, mus_file,  pack.mus_files);
        //ID_CHECK(SfxFile  &, sfx_file,  pack.sfx_files);
        ID_CHECK(GfxFrame &, gfx_frame, pack.gfx_frames);
        ID_CHECK(GfxAnim  &, gfx_anim,  pack.gfx_anims);
        ID_CHECK(Material &, material,  pack.materials);
        ID_CHECK(Sprite   &, sprite,    pack.sprites);
        ID_CHECK(TileType &, tile_type, pack.tile_types);
#undef ID_CHECK

        return err;
    }

    Err LoadResources(Pack &pack)
    {
        Err err = RN_SUCCESS;

#if 0
        for (GfxFile &gfx_file : pack.gfx_files) {
            if (!gfx_file.data_buffer.length) {
                ReadFileIntoDataBuffer(gfx_file.path, gfx_file.data_buffer);
            }
            Image img = LoadImageFromMemory(".png", gfx_file.data_buffer.bytes, gfx_file.data_buffer.length);
            gfx_file.texture = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        for (MusFile &mus_file : pack.mus_files) {
            if (!mus_file.data_buffer.length) {
                ReadFileIntoDataBuffer(mus_file.path, mus_file.data_buffer);
            }
            mus_file.music = LoadMusicStreamFromMemory(".wav", mus_file.data_buffer.bytes, mus_file.data_buffer.length);
        }
        for (SfxFile &sfx_file : pack.sfx_files) {
            if (!sfx_file.data_buffer.length) {
                ReadFileIntoDataBuffer(sfx_file.path, sfx_file.data_buffer);
            }
            Wave wav = LoadWaveFromMemory(".wav", sfx_file.data_buffer.bytes, sfx_file.data_buffer.length);
            sfx_file.sound = LoadSoundFromWave(wav);
            UnloadWave(wav);
        }
#endif

        for (GfxFile &gfxFile : pack.gfx_files) {
            if (gfxFile.path.empty()) continue;
            gfxFile.texture = LoadTexture(gfxFile.path.c_str());
        }
        for (MusFile &musFile : pack.mus_files) {
            if (musFile.path.empty()) continue;
            musFile.music = LoadMusicStream(musFile.path.c_str());
        }
        for (SfxFile &sfxFile : pack.sfx_files) {
            if (sfxFile.path.empty()) continue;
            sfxFile.sound = LoadSound(sfxFile.path.c_str());
        }

        // Generate checkerboard image in slot 0 as a placeholder for when other images fail to load
        Image placeholderImg = GenImageChecked(16, 16, 4, 4, MAGENTA, WHITE);
        if (placeholderImg.width) {
            Texture placeholderTex = LoadTextureFromImage(placeholderImg);
            if (placeholderTex.width) {
                //pack.gfx_files[0].id = GFX_FILE_NONE;
                pack.gfx_files[0].texture = placeholderTex;
                //pack.gfx_frames[0].id = GFX_FRAME_NONE;
                //pack.gfx_frames[0].gfx = GFX_FILE_NONE;
                pack.gfx_frames[0].w = placeholderTex.width;
                pack.gfx_frames[0].h = placeholderTex.height;
            } else {
                printf("[data] WARN: Failed to generate placeholder texture\n");
            }
            UnloadImage(placeholderImg);
        } else {
            printf("[data] WARN: Failed to generate placeholder image\n");
        }

        return err;
    }

    Err LoadPack(Pack &pack)
    {
        Err err = RN_SUCCESS;

        PackStream stream{};
        stream.mode = PACK_MODE_READ;
        stream.f = fopen(pack.path.c_str(), "rb");
        if (!stream.f) {
            return RN_BAD_FILE_READ;
        }
        stream.process = (ProcessFn)fread;
        stream.pack = &pack;

        do {
            err = Process(stream);
            if (err) break;

            err = Validate(*stream.pack);
            if (err) break;

            err = LoadResources(*stream.pack);
            if (err) break;
        } while(0);

        if (stream.f) fclose(stream.f);
        return err;
    }

    void UnloadPack(Pack &pack)
    {
        for (GfxFile &gfxFile : pack.gfx_files) {
            UnloadTexture(gfxFile.texture);
            FreeDataBuffer(gfxFile.data_buffer);
        }
        for (MusFile &musFile : pack.mus_files) {
            UnloadMusicStream(musFile.music);
            FreeDataBuffer(musFile.data_buffer);
        }
        for (SfxFile &sfxFile : pack.sfx_files) {
            UnloadSound(sfxFile.sound);
            FreeDataBuffer(sfxFile.data_buffer);
        }
    }

    void PlaySound(std::string id, float pitchVariance)
    {
        SfxFile *sfx_file = packs[0]->FindSound(id);
        if (!sfx_file) return;

        float variance = pitchVariance ? pitchVariance : sfx_file->pitch_variance;
        SetSoundPitch(sfx_file->sound, 1.0f + GetRandomFloatVariance(variance));

        printf("updatesprite playsound %s (multi = %d)\n", sfx_file->path.c_str(), sfx_file->multi);
        if (sfx_file->multi) {
            PlaySoundMulti(sfx_file->sound);
        } else if (!IsSoundPlaying(sfx_file->sound)) {
            PlaySound(sfx_file->sound);
        }
    }

    bool IsSoundPlaying(std::string id)
    {
        bool playing = false;

        SfxFile *sfx_file = packs[0]->FindSound(id);
        if (sfx_file) {
            playing = IsSoundPlaying(sfx_file->sound);
        }
        return playing;
    }

    void StopSound(std::string id)
    {
        SfxFile *sfx_file = packs[0]->FindSound(id);
        if (sfx_file) {
            StopSound(sfx_file->sound);
        }
    }

    const GfxFrame &GetSpriteFrame(const Entity &entity)
    {
        const Sprite sprite = packs[0]->sprites[entity.sprite];

        const GfxAnimId anim_id = sprite.anims[entity.direction];
        const GfxAnim &anim = packs[0]->gfx_anims[anim_id];

        const GfxFrameId frame_id = anim.frames[entity.anim_frame];
        const GfxFrame &frame = packs[0]->gfx_frames[frame_id];

        return frame;
    }
    void UpdateSprite(Entity &entity, double dt, bool newlySpawned)
    {
        entity.anim_accum += dt;

        // TODO: Make this more general and stop taking in entityType.
        switch (entity.type) {
            case ENTITY_PLAYER: case ENTITY_NPC: {
                if (entity.velocity.x > 0) {
                    entity.direction = data::DIR_E;
                } else {
                    entity.direction = data::DIR_W;
                }
                break;
            }
        }

        const Sprite sprite = packs[0]->sprites[entity.sprite];
        const GfxAnimId anim_id = sprite.anims[entity.direction];
        const GfxAnim &anim = packs[0]->gfx_anims[anim_id];
        const double anim_frame_time = (1.0 / anim.frame_rate) * anim.frame_delay;
        if (entity.anim_accum >= anim_frame_time) {
            entity.anim_frame++;
            if (entity.anim_frame >= anim.frame_count) {
                entity.anim_frame = 0;
            }
            entity.anim_accum -= anim_frame_time;
        }

        if (newlySpawned) {
            SfxFile *sfx_file = packs[0]->FindSound(anim.sound);
            if (sfx_file) {
                PlaySound(sfx_file->id);
            }
        }
    }
    void ResetSprite(Entity &entity)
    {
        const Sprite sprite = packs[0]->sprites[entity.sprite];
        const GfxAnimId anim_id = sprite.anims[entity.direction];
        const GfxAnim &anim = packs[0]->gfx_anims[anim_id];

        const SfxFile *sfx_file = packs[0]->FindSound(anim.sound);
        if (sfx_file && IsSoundPlaying(sfx_file->sound)) {
            StopSound(sfx_file->sound);
        }

        entity.anim_frame = 0;
        entity.anim_accum = 0;
    }
    void DrawSprite(const Entity &entity, Vector2 pos)
    {
        const GfxFrame &frame = GetSpriteFrame(entity);
        const GfxFile &file = packs[0]->gfx_files[frame.gfx];
        const Rectangle frame_rec{ (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h };
        //pos.x = floor(pos.x);
        //pos.y = floor(pos.y);
        DrawTextureRec(file.texture, frame_rec, pos, WHITE);
    }
}