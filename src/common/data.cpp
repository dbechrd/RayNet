#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include <sstream>
#include <iomanip>
#include <type_traits>

namespace data {
#define ENUM_STR_GENERATOR(type, enumDef, enumGen) \
    const char *type##Str(type id) {               \
        switch (id) {                              \
            enumDef(enumGen)                       \
            default: return "<unknown>";           \
        }                                          \
    }

    ENUM_STR_GENERATOR(DataType,   DATA_TYPES   , ENUM_GEN_CASE_RETURN_DESC);
    ENUM_STR_GENERATOR(SpriteId,   SPRITE_IDS,    ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(TileTypeId, TILE_TYPE_IDS, ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(EntityType, ENTITY_TYPES , ENUM_GEN_CASE_RETURN_STR);
#undef ENUM_STR_GENERATOR

    Texture placeholderTex{};
    std::vector<Pack *> packs{};

    struct GameState {
        bool freye_introduced;
    };
    GameState game_state{};

    GhostSnapshot::GhostSnapshot(Msg_S_EntitySnapshot &msg)
    {
        server_time              = msg.server_time;
        last_processed_input_cmd = msg.last_processed_input_cmd;

        // Entity
        map_name = msg.map_name;
        position = msg.position;

        // Life
        hp_max   = msg.hp_max;
        hp       = msg.hp;

        // Physics
        //speed    = msg.speed;
        velocity = msg.velocity;
    }

    Err LoadResources(Pack &pack);

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
        int bytes_read = 0;
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

    Err LoadGraphicsIndex(std::string path, std::vector<GfxFile> &gfx_files)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                GfxFile gfx_file{};
                if (iss
                    >> gfx_file.id
                    >> std::quoted(gfx_file.path)
                ) {
                    gfx_files.push_back(gfx_file);
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }
    Err LoadFramesIndex(std::string path, std::vector<GfxFrame> &gfx_frames)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                GfxFrame gfx_frame{};
                if (iss
                    >> gfx_frame.id
                    >> gfx_frame.gfx
                    >> gfx_frame.x
                    >> gfx_frame.y
                    >> gfx_frame.w
                    >> gfx_frame.h
                ) {
                    gfx_frames.push_back(gfx_frame);
                }
            }
            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }
    Err LoadAnimIndex(std::string path, std::vector<GfxAnim> &gfx_anims)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                int frame_rate = 0;
                int frame_count = 0;
                int frame_delay = 0;

                GfxAnim gfx_anim{};
                if (iss
                    >> gfx_anim.id
                    >> gfx_anim.sound
                    >> frame_rate
                    >> frame_count
                    >> frame_delay
                ) {
                    if (gfx_anim.sound == "-") gfx_anim.sound = "";

                    assert(frame_rate > 0 && frame_rate < UINT8_MAX);
                    assert(frame_count > 0 && frame_count < UINT8_MAX);
                    assert(frame_delay >= 0 && frame_delay < UINT8_MAX);
                    gfx_anim.frame_rate  = CLAMP(frame_rate, 0, UINT8_MAX);
                    gfx_anim.frame_count = CLAMP(frame_count, 0, UINT8_MAX);
                    gfx_anim.frame_delay = CLAMP(frame_delay, 0, UINT8_MAX);

                    std::string frame;
                    while (iss >> frame && frame[0] != '#') {
                        gfx_anim.frames.push_back(frame);
                    }
                    if (gfx_anim.frames.size() != gfx_anim.frame_count) {
                        assert(!"number of frames does not match frame count");
                        err = RN_BAD_ALLOC;
                    }

                    gfx_anims.push_back(gfx_anim);
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }
    Err LoadMusicIndex(std::string path, std::vector<MusFile> &mus_files)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                MusFile mus_file{};
                if (iss
                    >> mus_file.id
                    >> std::quoted(mus_file.path)
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
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                SfxFile sfx_file{};
                if (iss
                    >> sfx_file.id
                    >> std::quoted(sfx_file.path)
                    >> sfx_file.variations
                    >> sfx_file.pitch_variance
                    >> sfx_file.max_instances
                ) {
                    if (sfx_file.variations > 1) {
                        const char *file_dir = GetDirectoryPath(sfx_file.path.c_str());
                        const char *file_name = GetFileNameWithoutExt(sfx_file.path.c_str());
                        const char *file_ext = GetFileExtension(sfx_file.path.c_str());
                        for (int i = 1; i <= sfx_file.variations; i++) {
                            // Build variant path, e.g. chicken_cluck.wav -> chicken_cluck_01.wav
                            const char *variant_path = TextFormat("%s/%s_%02d%s", file_dir, file_name, i, file_ext);
                            SfxFile sfx_variant = sfx_file;
                            sfx_variant.path = variant_path;
                            sfx_files.push_back(sfx_variant);
                        }
                    } else {
                        sfx_files.push_back(sfx_file);
                    }
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }
    Err LoadMaterialIndex(std::string path, std::vector<Material> &materials)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line[0] == '#') continue;
                std::istringstream iss(line);

                Material material{};
                if (iss
                    >> material.id
                    >> material.footstep_sound
                ) {
                    materials.push_back(material);
                }
            }

            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }
    Err LoadTilemapIndex(std::string path, std::vector<TileMapData> &tile_maps)
    {
        Err err = RN_SUCCESS;

        std::ifstream inputFile(path);
        if (inputFile.is_open()) {
            TileMapData tile_map{};

            std::string line;
            while (!err && std::getline(inputFile, line)) {
                if (line.empty() || line[0] == '#') continue;
                std::istringstream iss(line);

                std::string type;
                iss >> type;
                if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                if (type == "map") {
                    iss >> tile_map.version
                        >> tile_map.name
                        >> tile_map.texture;
                    if (iss.fail()) { err = RN_BAD_FILE_READ; break; }
                } else if (type == "tile_defs") {
                    int tile_def_count = 0;
                    iss >> tile_def_count;
                    if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                    tile_map.tileDefs.reserve(tile_def_count);
                    for (int i = 0; i < tile_def_count; i++) {
                        std::getline(inputFile, line);
                        iss = std::istringstream(line);

                        int w = 0;
                        int h = 0;
                        int collide = 0;
                        int auto_tile_mask = 0;

                        TileDef tile_def{};
                        iss >> tile_def.x
                            >> tile_def.y
                            >> w
                            >> h
                            >> collide
                            >> auto_tile_mask;
                        if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                        tile_def.w = w;
                        tile_def.h = h;
                        tile_def.collide = collide;
                        tile_def.auto_tile_mask = auto_tile_mask;

                        tile_map.tileDefs.push_back(tile_def);
                    }
                } else if (type == "tiles") {
                    iss >> tile_map.width;
                    iss >> tile_map.height;
                    if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                    tile_map.tiles.reserve(tile_map.width * tile_map.height);
                    for (int y = 0; y < tile_map.height; y++) {
                        std::getline(inputFile, line);
                        iss = std::istringstream(line);

                        for (int x = 0; x < tile_map.width; x++) {
                            int tile = 0;
                            iss >> tile;
                            if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                            tile_map.tiles.push_back(tile);
                        }
                    }
                } else if (type == "path_nodes") {
                    int count = 0;
                    iss >> count;
                    if (iss.fail()) { err = RN_BAD_FILE_READ; break; }

                    tile_map.pathNodes.reserve(count);
                    for (int i = 0; i < count; i++) {
                        std::getline(inputFile, line);
                        iss = std::istringstream(line);

                        AiPathNode path_node{};
                        iss >> path_node.pos.x
                            >> path_node.pos.y
                            >> path_node.pos.z
                            >> path_node.waitFor;
                        if (iss.fail()) { err = RN_BAD_FILE_READ; break; }
                        tile_map.pathNodes.push_back(path_node);
                    }
                } else if (type == "path") {
                    AiPath path{};
                    iss >> path.pathNodeStart
                        >> path.pathNodeCount;
                    if (iss.fail()) { err = RN_BAD_FILE_READ; break; }
                    tile_map.paths.push_back(path);
                } else {
                    assert(!"unexpected line");
                    err = RN_BAD_FILE_READ;
                    break;
                }
            }

            if (!err) {
                tile_maps.push_back(tile_map);
            }
            inputFile.close();
        } else {
            assert(!"failed to read file");
            err = RN_BAD_FILE_READ;
        }

        return err;
    }

    Err SaveTilemap(std::string path, Tilemap &tilemap)
    {
        Err err = RN_SUCCESS;
#if 0
        std::ofstream stream(path);

        do {
            if (!stream.is_open()) {
                err = RN_BAD_FILE_WRITE; break;
            }

            stream << "version " << tilemap.version << '\n';
            stream << "size " << tilemap.width << " " << tilemap.height << '\n';
            stream << "name " << tilemap.name << '\n';
            stream << "texture " << tilemap.texture << '\n';
        } while (0);

        stream.close();
#else
        FILE *file = fopen(path.c_str(), "w");

        do {
            if (ferror(file)) {
                err = RN_BAD_FILE_WRITE; break;
            }

            fprintf(file, "%-10s %u\n", "version", tilemap.version);
            fprintf(file, "%-10s %u %u\n", "size", tilemap.width, tilemap.height);
            fprintf(file, "%-10s %s\n", "name", tilemap.name.c_str());
            fprintf(file, "%-10s %s\n", "texture", tilemap.texture.c_str());

            fprintf(file, "# x y w h collide auto_tile_mask\n");
            for (const TileDef &tile_def : tilemap.tileDefs) {
                fprintf(file, "%-10s %u %u %u %u %u %u\n",
                    "tile_def",
                    (uint32_t)tile_def.x,
                    (uint32_t)tile_def.y,
                    (uint32_t)tile_def.w,
                    (uint32_t)tile_def.h,
                    (uint32_t)tile_def.collide,
                    (uint32_t)tile_def.auto_tile_mask
                );

            }

            fprintf(file, "tiles_start\n");
            for (int i = 0; i < tilemap.tiles.size(); i++) {
                const Tile &tile = tilemap.tiles[i];
                fprintf(file, "%3u", tilemap.tiles[i]);
                if (i % tilemap.width == tilemap.width - 1) {
                    fprintf(file, "\n");
                } else {
                    fprintf(file, " ");
                }

            }
            fprintf(file, "tiles_end\n");

            fprintf(file, "# x y z wait_for\n");
            for (const AiPathNode &path_node : tilemap.pathNodes) {
                fprintf(file, "%-10s %f %f %f %f\n",
                    "path_node",
                    path_node.pos.x,
                    path_node.pos.y,
                    path_node.pos.z,
                    (float)path_node.waitFor
                );
            }

            // TODO(dlb): This doesn't need to exist.. Just have offset/count of AiPath point directly to path_nodes!??
            fprintf(file, "path_node_indices");
            for (uint32_t path_node_idx : tilemap.pathNodeIndices) {
                fprintf(file, " %u", path_node_idx);
            }
            fprintf(file, "\n");

            fprintf(file, "# node_offset node_count\n");
            for (const AiPath &path : tilemap.paths) {
                fprintf(file, "%-10s %u %u\n",
                    "path",
                    path.pathNodeStart,
                    path.pathNodeCount
                );
            }
        } while (0);

        if (file) fclose(file);
#endif
        return err;
    }

    void Init(void)
    {
        Err err = RN_SUCCESS;

        // Generate checkerboard image in slot 0 as a placeholder for when other images fail to load
        Image placeholderImg = GenImageChecked(16, 16, 4, 4, MAGENTA, WHITE);
        if (placeholderImg.width) {
            placeholderTex = LoadTextureFromImage(placeholderImg);
            if (placeholderTex.width) {
                //pack.gfx_files[0].id = GFX_FILE_NONE;
                //pack.gfx_files[0].texture = placeholderTex;

                //pack.gfx_frames[0].id = GFX_FRAME_NONE;
                //pack.gfx_frames[0].gfx = GFX_FILE_NONE;
                //pack.gfx_frames[0].w = placeholderTex.width;
                //pack.gfx_frames[0].h = placeholderTex.height;
            } else {
                printf("[data] WARN: Failed to generate placeholder texture\n");
            }
            UnloadImage(placeholderImg);
        } else {
            printf("[data] WARN: Failed to generate placeholder image\n");
        }

#if 1
        Sprite sprites[] = {
            // id                    anims
            //                       N                        E                         S       W                         NE      SE      SW      NW
            { SPRITE_CHR_MAGE,     { ""    ,                  "gfx_anim_chr_mage_e",    ""    , "gfx_anim_chr_mage_w",    ""    , ""    , ""    , ""     }},
            { SPRITE_NPC_LILY,     { ""    ,                  "gfx_anim_npc_lily_e",    ""    , "gfx_anim_npc_lily_w",    ""    , ""    , ""    , ""     }},
            { SPRITE_NPC_FREYE,    { ""    ,                  "gfx_anim_npc_freye_e",   ""    , "gfx_anim_npc_freye_w",   ""    , ""    , ""    , ""     }},
            { SPRITE_NPC_NESSA,    { ""    ,                  "gfx_anim_npc_nessa_e",   ""    , "gfx_anim_npc_nessa_w",   ""    , ""    , ""    , ""     }},
            { SPRITE_NPC_ELANE,    { ""    ,                  "gfx_anim_npc_elane_e",   ""    , "gfx_anim_npc_elane_w",   ""    , ""    , ""    , ""     }},
            { SPRITE_NPC_CHICKEN,  { ""    ,                  "gfx_anim_npc_chicken_e", ""    , "gfx_anim_npc_chicken_w", ""    , ""    , ""    , ""     }},
            { SPRITE_OBJ_CAMPFIRE, { "gfx_anim_obj_campfire", "",                       ""    , ""    ,                   ""    , ""    , ""    , ""     }},
            { SPRITE_PRJ_FIREBALL, { "gfx_anim_prj_fireball", "",                       ""    , ""    ,                   ""    , ""    , ""    , ""     }},
        };

        TileType tile_types[] = {
            // id               gfx/anim                  material     auto_mask   flags
            { TILE_GRASS,      "gfx_anim_til_grass",      "mat_grass", 0b00000000, TILE_FLAG_WALK },
            { TILE_STONE_PATH, "gfx_anim_til_stone_path", "mat_stone", 0b00000000, TILE_FLAG_WALK },
            { TILE_WATER,      "gfx_anim_til_water",      "mat_water", 0b00000000, TILE_FLAG_SWIM },
            { TILE_WALL,       "gfx_anim_til_wall",       "mat_stone", 0b00000000, TILE_FLAG_COLLIDE },

#if 0
            { TILE_AUTO_GRASS_00, 0b00000010 },
            { TILE_AUTO_GRASS_01, 0b01000010 },
            { TILE_AUTO_GRASS_02, 0b01000000 },
            { TILE_AUTO_GRASS_03, 0b00000000 },

            { TILE_AUTO_GRASS_04, 0b00001010 },
            { TILE_AUTO_GRASS_05, 0b01001010 },
            { TILE_AUTO_GRASS_06, 0b01001000 },
            { TILE_AUTO_GRASS_07, 0b00001000 },

            { TILE_AUTO_GRASS_08, 0b00011010 },
            { TILE_AUTO_GRASS_09, 0b01011010 },
            { TILE_AUTO_GRASS_10, 0b01011000 },
            { TILE_AUTO_GRASS_11, 0b00011000 },

            { TILE_AUTO_GRASS_12, 0b00010010 },
            { TILE_AUTO_GRASS_13, 0b01010010 },
            { TILE_AUTO_GRASS_14, 0b01010000 },
            { TILE_AUTO_GRASS_15, 0b00010000 },

            { TILE_AUTO_GRASS_16, 0b11011010 },
            { TILE_AUTO_GRASS_17, 0b01001011 },
            { TILE_AUTO_GRASS_18, 0b01101010 },
            { TILE_AUTO_GRASS_19, 0b01011110 },

            { TILE_AUTO_GRASS_20, 0b00011011 },
            { TILE_AUTO_GRASS_21, 0b01111111 },
            { TILE_AUTO_GRASS_22, 0b11111011 },
            { TILE_AUTO_GRASS_23, 0b01111000 },

            { TILE_AUTO_GRASS_24, 0b00011110 },
            { TILE_AUTO_GRASS_25, 0b11011111 },
            { TILE_AUTO_GRASS_26, 0b11111110 },
            { TILE_AUTO_GRASS_27, 0b11011000 },

            { TILE_AUTO_GRASS_28, 0b01111010 },
            { TILE_AUTO_GRASS_29, 0b01010110 },
            { TILE_AUTO_GRASS_30, 0b11010010 },
            { TILE_AUTO_GRASS_31, 0b01011011 },

            { TILE_AUTO_GRASS_32, 0b00001011 },
            { TILE_AUTO_GRASS_33, 0b01101011 },
            { TILE_AUTO_GRASS_34, 0b01111011 },
            { TILE_AUTO_GRASS_35, 0b01101000 },

            { TILE_AUTO_GRASS_36, 0b01011111 },
            { TILE_AUTO_GRASS_37, 0b01111110 },
            { TILE_AUTO_GRASS_38, 0b11111111 },
            { TILE_AUTO_GRASS_39, 0b11111000 },

            { TILE_AUTO_GRASS_40, 0b00011111 },
            { TILE_AUTO_GRASS_41, 0b00000000 },
            { TILE_AUTO_GRASS_42, 0b11011011 },
            { TILE_AUTO_GRASS_43, 0b11111010 },

            { TILE_AUTO_GRASS_44, 0b00010110 },
            { TILE_AUTO_GRASS_45, 0b11011110 },
            { TILE_AUTO_GRASS_46, 0b11010110 },
            { TILE_AUTO_GRASS_47, 0b11010000 },
#endif
        };

        std::vector<GfxFile> gfx_files{};
        err = LoadGraphicsIndex("resources/graphics.txt", gfx_files);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load graphics index.\n");
        }

        std::vector<GfxFrame> gfx_frames{};
        err = LoadFramesIndex("resources/graphics_frames.txt", gfx_frames);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load graphics frame index.\n");
        }

        std::vector<GfxAnim> gfx_anims{};
        err = LoadAnimIndex("resources/graphics_anims.txt", gfx_anims);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load graphics animation index.\n");
        }

        std::vector<MusFile> mus_files{};
        err = LoadMusicIndex("resources/music.txt", mus_files);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load music index.\n");
        }

        std::vector<SfxFile> sfx_files{};
        err = LoadSoundIndex("resources/sounds.txt", sfx_files);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load sound index.\n");
        }

        std::vector<Material> materials{};
        err = LoadMaterialIndex("resources/materials.txt", materials);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load material index.\n");
        }

        std::vector<TileMapData> tile_maps{};
        err = LoadTilemapIndex("resources/map/overworld_map.txt", tile_maps);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load tile map index.\n");
        }

        Pack packHardcoded{ "pack/pack1.dat" };
        for (auto &i : gfx_files)  packHardcoded.gfx_files .push_back(i);
        for (auto &i : mus_files)  packHardcoded.mus_files .push_back(i);
        for (auto &i : sfx_files)  packHardcoded.sfx_files .push_back(i);
        for (auto &i : gfx_frames) packHardcoded.gfx_frames.push_back(i);
        for (auto &i : gfx_anims)  packHardcoded.gfx_anims .push_back(i);
        for (auto &i : materials)  packHardcoded.materials .push_back(i);
        for (auto &i : sprites)    packHardcoded.sprites   .push_back(i);
        for (auto &i : tile_types) packHardcoded.tile_types.push_back(i);
        for (auto &i : tile_maps)  packHardcoded.tile_maps .push_back(i);

#if 0
        Pack packOverworld("pack/overworld_map.dat");
        err = LoadPack(packOverworld);
        assert(!err);
        // HACK: Re-scale path nodes for old maps
        //for (auto &pathNode : packOverworld.tile_maps[1].pathNodes) {
        //    pathNode.pos = Vector3Scale(pathNode.pos, 2);
        //}
        //err = SavePack(packOverworld);
        //assert(!err);
        packHardcoded.tile_maps.push_back(packOverworld.tile_maps[1]);
#endif

        //SaveTilemap("resources/map/overworld_map.txt", packHardcoded.tile_maps[1]);

        // Ensure every array element is initialized and in contiguous order by id
#define ID_CHECK(type, name, arr)                       \
            for (int i = 0; i < arr.size(); i++) {      \
                const type name = arr[i];               \
                if (name.id != i) {                     \
                    assert(!"expected contiguous IDs"); \
                }                                       \
            }

        ID_CHECK(Sprite   &, sprite,    packHardcoded.sprites);
        ID_CHECK(TileType &, tile_type, packHardcoded.tile_types);
#undef ID_CHECK

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

#define PROC(v) Process(stream, v);
    template <typename T>
    void Process(PackStream &stream, T &v)
    {
        static_assert(std::is_fundamental_v<T> || std::is_enum_v<T>,
            "You cannot fread/write a non-primitive type");
        stream.process(&v, sizeof(v), 1, stream.f);
    }
    void Process(PackStream &stream, std::string &str)
    {
        uint16_t strLen = (uint16_t)str.size();
        PROC(strLen);
        str.resize(strLen);
        stream.process(str.data(), 1, strLen, stream.f);
    }
    void Process(PackStream &stream, Vector2 &vec)
    {
        PROC(vec.x);
        PROC(vec.y);
    }
    void Process(PackStream &stream, Vector3 &vec)
    {
        PROC(vec.x);
        PROC(vec.y);
        PROC(vec.z);
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
        Process(stream, gfx_file.id);
        Process(stream, gfx_file.path);
        Process(stream, gfx_file.data_buffer);
        if (stream.mode == PACK_MODE_READ && !gfx_file.data_buffer.length && !gfx_file.path.empty()) {
            ReadFileIntoDataBuffer(gfx_file.path.c_str(), gfx_file.data_buffer);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->gfx_file_by_id[gfx_file.id] = index;
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
            stream.pack->mus_file_by_id[mus_file.id] = index;
        }
    }
    void Process(PackStream &stream, SfxFile &sfx_file, int index)
    {
        Process(stream, sfx_file.id);
        Process(stream, sfx_file.path);
        Process(stream, sfx_file.data_buffer);
        PROC(sfx_file.variations);
        PROC(sfx_file.pitch_variance);
        PROC(sfx_file.max_instances);

        if (stream.mode == PACK_MODE_READ) {
            if (!sfx_file.data_buffer.length && !sfx_file.path.empty()) {
                ReadFileIntoDataBuffer(sfx_file.path.c_str(), sfx_file.data_buffer);
            }

            auto &sfx_variants = stream.pack->sfx_file_by_id[sfx_file.id];
            sfx_variants.push_back(index);
        }
    }
    void Process(PackStream &stream, GfxFrame &gfx_frame, int index)
    {
        Process(stream, gfx_frame.id);
        Process(stream, gfx_frame.gfx);
        PROC(gfx_frame.x);
        PROC(gfx_frame.y);
        PROC(gfx_frame.w);
        PROC(gfx_frame.h);

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->gfx_frame_by_id[gfx_frame.id] = index;
        }
    }
    void Process(PackStream &stream, GfxAnim &gfx_anim, int index)
    {
        PROC(gfx_anim.id);
        Process(stream, gfx_anim.sound);
        PROC(gfx_anim.frame_rate);
        PROC(gfx_anim.frame_count);
        PROC(gfx_anim.frame_delay);
        gfx_anim.frames.resize(gfx_anim.frame_count);
        for (int i = 0; i < gfx_anim.frame_count; i++) {
            PROC(gfx_anim.frames[i]);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->gfx_anim_by_id[gfx_anim.id] = index;
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
        PROC(entity.type);
        PROC(entity.spawned_at);
        //PROC(entity.despawned_at);

        Process(stream, entity.map_name);
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

        //PROC(entity.force_accum);
        //PROC(entity.velocity);

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
    void Process(PackStream &stream, TileMapData &tile_map, int index)
    {
        uint32_t sentinel = 0;
        if (stream.mode == PackStreamMode::PACK_MODE_WRITE) {
            sentinel = Tilemap::SENTINEL;
        }

        if (stream.mode == PackStreamMode::PACK_MODE_WRITE) {
            assert(tile_map.width * tile_map.height == tile_map.tiles.size());
        }

        PROC(tile_map.version);
        Process(stream, tile_map.name);
        Process(stream, tile_map.texture);
        PROC(tile_map.width);
        PROC(tile_map.height);

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        //SetTextureWrap(texEntry.texture, TEXTURE_WRAP_CLAMP);

        //if (!width || !height) {
        //    err = RN_INVALID_SIZE; break;
        //}

        uint32_t tileDefCount       = tile_map.tileDefs.size();
        uint32_t pathNodeCount      = tile_map.pathNodes.size();
        uint32_t pathNodeIndexCount = tile_map.pathNodeIndices.size();
        uint32_t pathCount          = tile_map.paths.size();

        PROC(tileDefCount);
        PROC(pathNodeCount);
        PROC(pathNodeIndexCount);
        PROC(pathCount);

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        tile_map.tileDefs       .resize(tileDefCount);
        tile_map.tiles          .resize(tile_map.width * tile_map.height);
        tile_map.pathNodes      .resize(pathNodeCount);
        tile_map.pathNodeIndices.resize(pathNodeIndexCount);
        tile_map.paths          .resize(pathCount);

        for (uint32_t i = 0; i < tileDefCount; i++) {
            data::TileDef &tileDef = tile_map.tileDefs[i];
            PROC(tileDef.x);
            PROC(tileDef.y);
            PROC(tileDef.w);
            PROC(tileDef.h);
            PROC(tileDef.collide);
            PROC(tileDef.auto_tile_mask);

            // TODO: Idk where/how to do this, but we don't have the texture
            //       loaded yet in this context, necessarily.
            //tileDef.color = GetImageColor(texEntry.image, tileDef.x, tileDef.y);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        for (Tile &tile : tile_map.tiles) {
            PROC(tile);
            if (tile >= tileDefCount) {
                tile = 0;
                //err = RN_OUT_OF_BOUNDS; break;
            }
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        for (data::AiPathNode &aiPathNode : tile_map.pathNodes) {
            PROC(aiPathNode.pos.x);
            PROC(aiPathNode.pos.y);
            if (tile_map.version >= 9) {
                PROC(aiPathNode.pos.z);
            }
            PROC(aiPathNode.waitFor);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        for (uint32_t &aiPathNodeIndex : tile_map.pathNodeIndices) {
            PROC(aiPathNodeIndex);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        for (data::AiPath &aiPath : tile_map.paths) {
            PROC(aiPath.pathNodeStart);
            PROC(aiPath.pathNodeCount);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        if (stream.mode == PACK_MODE_READ) {
            tile_map.id = stream.pack->next_map_id++;  // HACK: Store this or something.. firebase?
            stream.pack->tile_map_by_id[tile_map.id] = index;
            stream.pack->tile_map_by_name[tile_map.name] = index;
        }
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
                /* 0 slot is reserved, skip it when writing */\
                for (int i = 1; i < arr.size(); i++) { \
                    auto &entry = arr[i]; \
                    pack.toc.entries.push_back(PackTocEntry(entry.dtype, ftell(stream.f))); \
                    Process(stream, entry, i); \
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
            WRITE_ARRAY(pack.tile_maps);
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

            pack.gfx_files .resize(1 + typeCounts[DAT_TYP_GFX_FILE]);
            pack.mus_files .resize(1 + typeCounts[DAT_TYP_MUS_FILE]);
            pack.sfx_files .resize(1 + typeCounts[DAT_TYP_SFX_FILE]);
            pack.gfx_frames.resize(1 + typeCounts[DAT_TYP_GFX_FRAME]);
            pack.gfx_anims .resize(1 + typeCounts[DAT_TYP_GFX_ANIM]);
            pack.materials .resize(1 + typeCounts[DAT_TYP_MATERIAL]);
            pack.sprites   .resize(1 + typeCounts[DAT_TYP_SPRITE]);
            pack.tile_types.resize(1 + typeCounts[DAT_TYP_TILE_TYPE]);
            pack.tile_maps .resize(1 + typeCounts[DAT_TYP_TILE_MAP]);
            pack.entities  .resize(1 + typeCounts[DAT_TYP_ENTITY]);

            int typeNextIndex[DAT_TYP_COUNT]{};
            // 0 slot is reserved, skip it when reading
            for (int i = 0; i < DAT_TYP_COUNT; i++) {
                typeNextIndex[i] = 1;
            }

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.f, tocEntry.offset, SEEK_SET);
                int index = typeNextIndex[tocEntry.dtype];
                tocEntry.index = index;
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  Process(stream, pack.gfx_files [index], index); break;
                    case DAT_TYP_MUS_FILE:  Process(stream, pack.mus_files [index], index); break;
                    case DAT_TYP_SFX_FILE:  Process(stream, pack.sfx_files [index], index); break;
                    case DAT_TYP_GFX_FRAME: Process(stream, pack.gfx_frames[index], index); break;
                    case DAT_TYP_GFX_ANIM:  Process(stream, pack.gfx_anims [index], index); break;
                    case DAT_TYP_MATERIAL:  Process(stream, pack.materials [index], index); break;
                    case DAT_TYP_SPRITE:    Process(stream, pack.sprites   [index], index); break;
                    case DAT_TYP_TILE_TYPE: Process(stream, pack.tile_types[index], index); break;
                    case DAT_TYP_ENTITY:    Process(stream, pack.entities  [index], index); break;
                    case DAT_TYP_TILE_MAP:  Process(stream, pack.tile_maps [index], index); break;
                }
                typeNextIndex[tocEntry.dtype]++;
            }
        }

        return err;
    }
#undef PROC

    Err SavePack(Pack &pack)
    {
        Err err = RN_SUCCESS;

        if (FileExists(pack.path.c_str())) {
            err = MakeBackup(pack.path.c_str());
            if (err) return err;
        }

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
            for (int i = 0; i < arr.size(); i++) {      \
                const type name = arr[i];               \
                if (name.id != i) {                     \
                    assert(!"expected contiguous IDs"); \
                    return RN_BAD_FILE_READ;            \
                }                                       \
            }

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

        for (GfxFile &gfx_file : pack.gfx_files) {
            if (gfx_file.path.empty()) continue;
            gfx_file.texture = LoadTexture(gfx_file.path.c_str());
            SetTextureFilter(gfx_file.texture, TEXTURE_FILTER_POINT);
        }
        for (MusFile &mus_file : pack.mus_files) {
            if (mus_file.path.empty()) continue;
            mus_file.music = LoadMusicStream(mus_file.path.c_str());
        }
        for (SfxFile &sfx_file : pack.sfx_files) {
            if (sfx_file.path.empty()) continue;
            sfx_file.sound = LoadSound(sfx_file.path.c_str());
            if (sfx_file.sound.frameCount) {
                sfx_file.instances.resize(sfx_file.max_instances);
                for (int i = 0; i < sfx_file.max_instances; i++) {
                    sfx_file.instances[i] = LoadSoundAlias(sfx_file.sound);
                }
            }
        }

        // Place checkerboard image in slot 0 as a placeholder for when other images fail to load
        assert(placeholderTex.width);

        assert(pack.gfx_files.size());
        assert(pack.gfx_files[0].id.empty());
        pack.gfx_files[0].texture = placeholderTex;

        assert(pack.gfx_frames.size());
        assert(pack.gfx_frames[0].id.empty());
        //pack.gfx_frames[0].id = GFX_FRAME_NONE;
        //pack.gfx_frames[0].gfx = GFX_FILE_NONE;
        pack.gfx_frames[0].w = placeholderTex.width;
        pack.gfx_frames[0].h = placeholderTex.height;

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
            assert(sfxFile.instances.size() == sfxFile.max_instances);
            for (int i = 0; i < sfxFile.max_instances; i++) {
                UnloadSoundAlias(sfxFile.instances[i]);
            }
            UnloadSound(sfxFile.sound);
            FreeDataBuffer(sfxFile.data_buffer);
        }
    }

    void PlaySound(std::string id, float pitchVariance)
    {
        const SfxFile &sfx_file = packs[0]->FindSoundVariant(id);
        assert(sfx_file.instances.size() == sfx_file.max_instances);

        //printf("updatesprite playsound %s (multi = %d)\n", sfx_file.path.c_str(), sfx_file.multi);
        for (int i = 0; i < sfx_file.max_instances; i++) {
            if (!IsSoundPlaying(sfx_file.instances[i])) {
                float variance = pitchVariance ? pitchVariance : sfx_file.pitch_variance;
                SetSoundPitch(sfx_file.instances[i], 1.0f + GetRandomFloatVariance(variance));
                PlaySound(sfx_file.instances[i]);
                break;
            }
        }
    }
    bool IsSoundPlaying(std::string id)
    {
        // TODO: Does this work still with SoundAlias stuff?
        const SfxFile &sfx_file = packs[0]->FindSoundVariant(id);
        assert(sfx_file.instances.size() == sfx_file.max_instances);

        bool playing = false;
        for (int i = 0; i < sfx_file.max_instances; i++) {
            playing = IsSoundPlaying(sfx_file.instances[i]);
            if (playing) break;
        }
        return playing;
    }
    void StopSound(std::string id)
    {
        const SfxFile &sfx_file = packs[0]->FindSoundVariant(id);
        assert(sfx_file.instances.size() == sfx_file.max_instances);

        for (int i = 0; i < sfx_file.max_instances; i++) {
            StopSound(sfx_file.instances[i]);
        }
    }

    const GfxFrame &GetSpriteFrame(const Entity &entity)
    {
        const Sprite sprite = packs[0]->sprites[entity.sprite];

        const std::string anim_id = sprite.anims[entity.direction];
        const GfxAnim &anim = packs[0]->FindGraphicAnim(anim_id);

        std::string frame_id = "";
        if (!anim.frames.empty()) {
            frame_id = anim.frames[entity.anim_frame];
        }
        const GfxFrame &frame = packs[0]->FindGraphicFrame(frame_id);

        return frame;
    }
    Rectangle GetSpriteRect(const Entity &entity)
    {
        const data::GfxFrame &frame = GetSpriteFrame(entity);
        const Rectangle rect{
            entity.position.x - (float)(frame.w / 2),
            entity.position.y - entity.position.z - (float)frame.h,
            (float)frame.w,
            (float)frame.h
        };
        return rect;
    }
    void UpdateSprite(Entity &entity, double dt, bool newlySpawned)
    {
        entity.anim_accum += dt;

        // TODO: Make this more general and stop taking in entityType.
        switch (entity.type) {
            case ENTITY_PLAYER: case ENTITY_NPC: {
                if (entity.velocity.x > 0) {
                    entity.direction = data::DIR_E;
                } else if (entity.velocity.x < 0) {
                    entity.direction = data::DIR_W;
                } else if (!entity.direction) {
                    entity.direction = data::DIR_E;  // HACK: we don't have north sprites atm
                }
                break;
            }
        }

        const Sprite sprite = packs[0]->sprites[entity.sprite];
        const GfxAnim &anim = packs[0]->FindGraphicAnim(sprite.anims[entity.direction]);
        const double anim_frame_time = (1.0 / anim.frame_rate) * anim.frame_delay;
        if (entity.anim_accum >= anim_frame_time) {
            entity.anim_frame++;
            if (entity.anim_frame >= anim.frame_count) {
                entity.anim_frame = 0;
            }
            entity.anim_accum -= anim_frame_time;
        }

        if (newlySpawned) {
            const SfxFile &sfx_file = packs[0]->FindSoundVariant(anim.sound);
            PlaySound(sfx_file.id);
        }
    }
    void ResetSprite(Entity &entity)
    {
        const Sprite sprite = packs[0]->sprites[entity.sprite];
        GfxAnim &anim = packs[0]->FindGraphicAnim(sprite.anims[entity.direction]);
        StopSound(anim.sound);

        entity.anim_frame = 0;
        entity.anim_accum = 0;
    }
    void DrawSprite(const Entity &entity)
    {
        const GfxFrame &frame = GetSpriteFrame(entity);
        const GfxFile &gfx_file = packs[0]->FindGraphic(frame.gfx);

        Rectangle sprite_rect = GetSpriteRect(entity);
        Vector3 pos = { sprite_rect.x, sprite_rect.y };
        if (Vector3LengthSqr(entity.velocity) < 0.0001f) {
            pos.x = floorf(pos.x);
            pos.y = floorf(pos.y);
        }

        const Vector2 sprite_pos{ pos.x, pos.y - pos.z };
        const Rectangle frame_rec{ (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h };
        DrawTextureRec(gfx_file.texture, frame_rec, sprite_pos, WHITE);
    }
}