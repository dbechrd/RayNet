#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#define MD_HIJACK_STRING_CONSTANTS 0
#include "md.h"
#include <sstream>
#include <iomanip>
#include <type_traits>

namespace data {
    struct GameState {
        bool freye_introduced;
    };

    Texture placeholderTex{};
    std::vector<Pack *> packs{};
    GameState game_state{};

    GhostSnapshot::GhostSnapshot(Msg_S_EntitySnapshot &msg)
    {
        server_time              = msg.server_time;
        last_processed_input_cmd = msg.last_processed_input_cmd;

        // Entity
        map_id   = msg.map_id;
        position = msg.position;

        // Life
        hp_max   = msg.hp_max;
        hp       = msg.hp;

        // Physics
        //speed    = msg.speed;
        velocity = msg.velocity;
    }

    void GenPlaceholderTexture()
    {
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
    }

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

    void ReadFileIntoDataBuffer(const std::string &filename, DatBuffer &datBuffer)
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

    Err SaveTilemap(const std::string &path, Tilemap &tilemap)
    {
        Err err = RN_SUCCESS;

        FILE *file = fopen(path.c_str(), "w");
        do {
            if (ferror(file)) {
                err = RN_BAD_FILE_WRITE; break;
            }

            fprintf(file, "@Tilemap %s: {\n", tilemap.id.c_str());
            fprintf(file, "    version: %u\n", tilemap.version);
            fprintf(file, "    width: %u\n", tilemap.width);
            fprintf(file, "    height: %u\n", tilemap.height);

            // tile_defs
            fprintf(file, "    tile_defs: [\n");
            int max_tile_def_id_len = 0;
            for (std::string &tile_def_id : tilemap.tileDefs) {
                max_tile_def_id_len = MAX(max_tile_def_id_len, tile_def_id.size());
            }
            for (std::string &tile_def_id : tilemap.tileDefs) {
                fprintf(file, "        %-*s\n", max_tile_def_id_len, tile_def_id.c_str());
            }
            fprintf(file, "    ]\n");

            // object_defs
            fprintf(file, "    object_defs: [\n");
            int max_object_def_id_len = 0;
            for (std::string &object_def_id : tilemap.objectDefs) {
                max_object_def_id_len = MAX(max_object_def_id_len, object_def_id.size());
            }
            for (std::string &object_def_id : tilemap.objectDefs) {
                fprintf(file, "        %-*s\n", max_object_def_id_len, object_def_id.c_str());
            }
            fprintf(file, "    ]\n");

            // tiles
            fprintf(file, "    tiles: [");
            for (int i = 0; i < tilemap.tiles.size(); i++) {
                if (i % tilemap.width == 0) {
                    fprintf(file, "\n        ");
                }
                fprintf(file, "%3u", tilemap.tiles[i]);
            }
            fprintf(file, "\n");
            fprintf(file, "    ]\n");

            // objects
            fprintf(file, "    objects: [");
            for (int i = 0; i < tilemap.objects.size(); i++) {
                if (i % tilemap.width == 0) {
                    fprintf(file, "\n        ");
                }
                fprintf(file, "%3u", tilemap.objects[i]);
            }
            fprintf(file, "\n");
            fprintf(file, "    ]\n");

            // path_nodes
            fprintf(file, "    path_nodes: [\n");
            fprintf(file, "        //%-6s %-6s %-6s %-4s\n", "x", "y", "z", "wait_for");
            for (AiPathNode &path_node : tilemap.pathNodes) {
                fprintf(file, "        { %-6.f %-6.f %-6.f %-4.1f }\n",
                    path_node.pos.x,
                    path_node.pos.y,
                    path_node.pos.z,
                    path_node.waitFor
                );
            }
            fprintf(file, "    ]\n");

            // paths
            fprintf(file, "    paths: [\n");
            fprintf(file, "        // node_start node_count\n");
            for (AiPath &path : tilemap.paths) {
                fprintf(file, "        { %u %u }\n",
                    path.pathNodeStart,
                    path.pathNodeCount
                );
            }
            fprintf(file, "    ]\n");

            fprintf(file, "}\n");
        } while (0);

        if (file) fclose(file);
        return err;
    }

    void dlb_MD_PrintDebugDumpFromNode(FILE *file, MD_Node *node, MD_GenerateFlags flags)
    {
        MD_ArenaTemp scratch = MD_GetScratch(0, 0);
        MD_String8List list = {0};
        MD_DebugDumpFromNode(scratch.arena, &list, node, 0, MD_S8Lit("    "), flags);
        MD_String8 string = MD_S8ListJoin(scratch.arena, list, 0);
        fwrite(string.str, string.size, 1, file);
        MD_ReleaseScratch(scratch);
    }
    void dlb_MD_PrintErrors(MD_ParseResult *parse_result)
    {
        for (MD_Message *message = parse_result->errors.first; message; message = message->next) {
            MD_CodeLoc code_loc = MD_CodeLocFromNode(message->node);
            MD_PrintMessage(stderr, code_loc, message->kind, message->string);
        }
    }
    bool dlb_MD_Expect_Ident(MD_Node *node, std::string *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_Identifier)) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected identifier."));
            return false;
        }
        if (result) {
            *result = std::string((char *)node->string.str, node->string.size);
        }
        return true;
    }
    bool dlb_MD_Expect_String(MD_Node *node, std::string *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_StringDoubleQuote)) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected double-quoted string."));
            return false;
        }
        if (result) {
            *result = std::string((char *)node->string.str, node->string.size);
        }
        return true;
    }
    bool dlb_MD_Expect_Int(MD_Node *node, int min, int max, int *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_Numeric) || !(MD_StringIsCStyleInt(node->string))) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected integer literal."));
            return false;
        }
        int val = (int)MD_CStyleIntFromString(node->string);
        if (val < min || val > max) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected integer in inclusive range [%d, %d].", min, max);
            return false;
        }
        if (result) {
            *result = val;
        }
        return true;
    }
    bool dlb_MD_Expect_Uint(MD_Node *node, uint32_t min, uint32_t max, uint32_t *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_Numeric) || !(MD_StringIsCStyleInt(node->string))) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected unsigned integer literal."));
            return false;
        }
        MD_u64 val = MD_U64FromString(node->string, 10);
        if (val < min || val > max) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected unsigned integer in inclusive range [%u, %u].", min, max);
            return false;
        }
        if (result) {
            *result = val;
        }
        return true;
    }
    bool dlb_MD_Expect_Float(MD_Node *node, float *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_Numeric)) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected float literal."));
            return false;
        }
        if (result) {
            *result = (float)MD_F64FromString(node->string);
        }
        return true;
    }
    bool dlb_MD_Expect_Double(MD_Node *node, double *result)
    {
        if (node->kind != MD_NodeKind_Main || !(node->flags & MD_NodeFlag_Numeric)) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected double literal."));
            return false;
        }
        if (result) {
            *result = MD_F64FromString(node->string);
        }
        return true;
    }
    bool dlb_MD_Expect_Nil(MD_Node *node)
    {
        if (!MD_NodeIsNil(node)) {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected end of scope.");
            return false;
        }
        return true;
    }

#define META_ID(dest) \
    if (!dlb_MD_Expect_Ident(node, &dest)) break;
#define META_IDENT(dest) \
        if (!dlb_MD_Expect_Ident(node, &dest)) break; \
        node = node->next;
#define META_STRING(dest) \
        if (!dlb_MD_Expect_String(node, &dest)) break; \
        node = node->next;
#define META_UINT8(dest) \
        do { \
            uint32_t val = 0; \
            if (!dlb_MD_Expect_Uint(node, 0, UINT8_MAX, &val)) break; \
            dest = (uint8_t)val; \
            node = node->next; \
        } while(0);
#define META_UINT16(dest) \
        do { \
            uint32_t val = 0; \
            if (!dlb_MD_Expect_Uint(node, 0, UINT16_MAX, &val)) break; \
            dest = (uint16_t)val; \
            node = node->next; \
        } while(0);
#define META_UINT32(dest) \
        do { \
            uint32_t val = 0; \
            if (!dlb_MD_Expect_Uint(node, 0, UINT32_MAX, &val)) break; \
            dest = (uint32_t)val; \
            node = node->next; \
        } while(0);
#define META_INT(dest) \
        if (!dlb_MD_Expect_Int(node, INT32_MIN, INT32_MAX, &dest)) break; \
        node = node->next;
#define META_FLOAT(dest) \
        if (!dlb_MD_Expect_Float(node, &dest)) break; \
        node = node->next;
#define META_DOUBLE(dest) \
        if (!dlb_MD_Expect_Double(node, &dest)) break; \
        node = node->next;
#define META_CHILDREN_BEGIN \
    do { \
        MD_Node *parent = node; \
        MD_Node *node = parent->first_child;
#define META_CHILDREN_END \
        dlb_MD_Expect_Nil(node); \
    } while(0);
#define META_CHILDREN_LOOP_BEGIN \
    while (!MD_NodeIsNil(node)) {

#define META_CHILDREN_LOOP_END \
    }
#define META_CHILDREN_LOOP_END_NEXT \
        node = node->next; \
    }

    void PackAddMeta(Pack &pack, const char *filename)
    {
        MD_Arena *arena = MD_ArenaAlloc();

        MD_String8 md_filename = MD_S8CString((char *)filename);
        MD_ParseResult parse_result = MD_ParseWholeFile(arena, md_filename);

        if (!parse_result.errors.node_count) {
            //dlb_MD_PrintDebugDumpFromNode(stdout, parse_result.node, MD_GenerateFlags_All);
            for (MD_EachNode(node, parse_result.node->first_child)) {
                auto tag_count = MD_TagCountFromNode(node);
                if (tag_count != 1) {
                    MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8CString((char *)"Expected exactly 1 tag."));
                    MD_MessageListPush(&parse_result.errors, md_err);

                    //MD_CodeLoc loc = MD_CodeLocFromNode(node);
                    //MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected exactly 1 tag.");
                    break;
                }

                if (MD_NodeHasTag(node, MD_S8Lit("GfxFile"), 0)) {
                    GfxFile gfx_file{};

                    META_ID(gfx_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(gfx_file.path);
                    META_CHILDREN_END;

                    pack.gfx_files.push_back(gfx_file);
                } else if (MD_NodeHasTag(node, MD_S8Lit("MusFile"), 0)) {
                    MusFile mus_file{};

                    META_ID(mus_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(mus_file.path);
                    META_CHILDREN_END;

                    pack.mus_files.push_back(mus_file);
                } else if (MD_NodeHasTag(node, MD_S8Lit("SfxFile"), 0)) {
                    SfxFile sfx_file{};

                    META_ID(sfx_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(sfx_file.path);
                        META_INT(sfx_file.variations);
                        META_FLOAT(sfx_file.pitch_variance);
                        META_INT(sfx_file.max_instances);
                    META_CHILDREN_END;

                    // NOTE(dlb): This is a bit janky, probably a better way to handle sound variants tbh
                    if (sfx_file.variations > 1) {
                        const char *file_dir = GetDirectoryPath(sfx_file.path.c_str());
                        const char *file_name = GetFileNameWithoutExt(sfx_file.path.c_str());
                        const char *file_ext = GetFileExtension(sfx_file.path.c_str());
                        for (int i = 1; i <= sfx_file.variations; i++) {
                            // Build variant path, e.g. chicken_cluck.wav -> chicken_cluck_01.wav
                            const char *variant_path = TextFormat("%s/%s_%02d%s", file_dir, file_name, i, file_ext);
                            SfxFile sfx_variant = sfx_file;
                            sfx_variant.path = variant_path;
                            pack.sfx_files.push_back(sfx_variant);
                        }
                    } else {
                        pack.sfx_files.push_back(sfx_file);
                    }
                } else if (MD_NodeHasTag(node, MD_S8Lit("GfxFrame"), 0)) {
                    GfxFrame gfx_frame{};

                    META_ID(gfx_frame.id);
                    META_CHILDREN_BEGIN;
                        META_IDENT(gfx_frame.gfx);
                        META_UINT16(gfx_frame.x);
                        META_UINT16(gfx_frame.y);
                        META_UINT16(gfx_frame.w);
                        META_UINT16(gfx_frame.h);
                    META_CHILDREN_END;

                        pack.gfx_frames.push_back(gfx_frame);
                } else if (MD_NodeHasTag(node, MD_S8Lit("GfxAnim"), 0)) {
                    GfxAnim gfx_anim{};

                    META_ID(gfx_anim.id);
                    META_CHILDREN_BEGIN;
                        META_IDENT(gfx_anim.sound);
                        META_UINT8(gfx_anim.frame_rate);
                        META_UINT8(gfx_anim.frame_count);
                        META_UINT8(gfx_anim.frame_delay);
                        gfx_anim.frames.resize(gfx_anim.frame_count);
                        for (int i = 0; i < gfx_anim.frame_count; i++) {
                            META_IDENT(gfx_anim.frames[i]);
                        }
                    META_CHILDREN_END;

                    pack.gfx_anims.push_back(gfx_anim);
                } else if (MD_NodeHasTag(node, MD_S8Lit("Material"), 0)) {
                    Material material{};

                    META_ID(material.id);
                    META_CHILDREN_BEGIN;
                        META_IDENT(material.footstep_sound);
                        int flag_walk = 0;
                        META_INT(flag_walk);
                        int flag_swim = 0;
                        META_INT(flag_swim);
                        if (flag_walk) material.flags |= MATERIAL_FLAG_WALK;
                        if (flag_swim) material.flags |= MATERIAL_FLAG_SWIM;
                    META_CHILDREN_END;

                    pack.materials.push_back(material);
                 } else if (MD_NodeHasTag(node, MD_S8Lit("Object"), 0)) {
                    Object object{};

                    META_ID(object.id);
                    META_CHILDREN_BEGIN;
                        META_IDENT(object.type);
                        if (object.type == "lootable") {
                            META_IDENT(object.animation);
                            META_IDENT(object.loot_table_id);
                        } else if (object.type == "warp") {
                            META_IDENT(object.animation);
                            META_IDENT(object.warp_map_id);
                            META_FLOAT(object.warp_dest_pos.x);
                            META_FLOAT(object.warp_dest_pos.y);
                            META_FLOAT(object.warp_dest_pos.z);
                        }
                    META_CHILDREN_END;

                    pack.objects.push_back(object);
                } else if (MD_NodeHasTag(node, MD_S8Lit("Sprite"), 0)) {
                    Sprite sprite{};

                    META_ID(sprite.id);
                    META_CHILDREN_BEGIN;
                        for (int i = 0; i < ARRAY_SIZE(sprite.anims); i++) {
                            META_IDENT(sprite.anims[i]);
                        }
                    META_CHILDREN_END;

                    pack.sprites.push_back(sprite);
                } else if (MD_NodeHasTag(node, MD_S8Lit("TileDef"), 0)) {
                    TileDef tile_def{};

                    META_ID(tile_def.id);
                    META_CHILDREN_BEGIN;
                    META_IDENT(tile_def.anim);
                    META_IDENT(tile_def.material);
                    META_UINT8(tile_def.auto_tile_mask);
                    META_CHILDREN_END;

                    pack.tile_defs.push_back(tile_def);
                } else if (MD_NodeHasTag(node, MD_S8Lit("Tilemap"), 0)) {
                    Tilemap map{};

                    META_ID(map.id);
                    META_CHILDREN_BEGIN;           // {}
                        META_CHILDREN_LOOP_BEGIN;  // key: value
                            std::string key{};
                            META_ID(key);          // key

                            META_CHILDREN_BEGIN;   // value
                            if (key == "version") {
                                META_UINT32(map.version);
                            } else if (key == "width") {
                                META_UINT32(map.width);
                            } else if (key == "height") {
                                META_UINT32(map.height);
                            } else if (key == "tile_defs") {
                                META_CHILDREN_LOOP_BEGIN; // []
                                    std::string tile_def_id{};
                                    META_ID(tile_def_id);
                                    map.tileDefs.push_back(tile_def_id);
                                META_CHILDREN_LOOP_END_NEXT;
                            } else if (key == "object_defs") {
                                META_CHILDREN_LOOP_BEGIN; // []
                                std::string object_def_id{};
                                META_ID(object_def_id);
                                map.objectDefs.push_back(object_def_id);
                                META_CHILDREN_LOOP_END_NEXT;
                            } else if (key == "tiles") {
                                META_CHILDREN_LOOP_BEGIN;  // []
                                    Tile tile{};

                                    META_UINT8(tile);

                                    map.tiles.push_back(tile);
                                META_CHILDREN_LOOP_END;
                            } else if (key == "objects") {
                                META_CHILDREN_LOOP_BEGIN;  // []
                                    uint8_t object{};

                                    META_UINT8(object);

                                    map.objects.push_back(object);
                                META_CHILDREN_LOOP_END;
                            } else if (key == "path_nodes") {
                                META_CHILDREN_LOOP_BEGIN;  // []
                                    AiPathNode path_node{};

                                    META_CHILDREN_BEGIN;  // {}
                                        META_FLOAT(path_node.pos.x);
                                        META_FLOAT(path_node.pos.y);
                                        META_FLOAT(path_node.pos.z);
                                        META_DOUBLE(path_node.waitFor);
                                    META_CHILDREN_END;

                                    map.pathNodes.push_back(path_node);
                                META_CHILDREN_LOOP_END_NEXT;
                            } else if (key == "paths") {
                                META_CHILDREN_LOOP_BEGIN;  // []
                                    AiPath path{};

                                    META_CHILDREN_BEGIN;  // {}
                                    META_UINT32(path.pathNodeStart);
                                    META_UINT32(path.pathNodeCount);
                                    META_CHILDREN_END;

                                    map.paths.push_back(path);
                                META_CHILDREN_LOOP_END_NEXT;
                            }
                            META_CHILDREN_END;
                        META_CHILDREN_LOOP_END_NEXT;
                    META_CHILDREN_END;

                    assert(map.tiles.size() == map.width * map.height);
                    if (map.objects.size() == 0) {
                        map.objects.resize(4096);
                    }
                    assert(map.objects.size() == map.width * map.height);
                    pack.tile_maps.push_back(map);
                }
            }
        }

        //MD_push
        //parse_result.errors

        if (parse_result.errors.node_count) {
            dlb_MD_PrintErrors(&parse_result);
            assert(!"MD parse failed");
        }
        MD_ArenaRelease(arena);
    }

#if 0
    void PackDebugPrint(Pack &pack)
    {
        printf("\n--- %s ---\n", pack.path.c_str());
        for (GfxFile &gfx_file : pack.gfx_files) {
            if (!gfx_file.id.size()) continue;
            printf("%s %s\n", gfx_file.id.c_str(), gfx_file.path.c_str());
        }
        for (MusFile &mus_file : pack.mus_files) {
            if (!mus_file.id.size()) continue;
            printf("%s %s\n", mus_file.id.c_str(), mus_file.path.c_str());
        }
        for (SfxFile &sfx_file : pack.sfx_files) {
            if (!sfx_file.id.size()) continue;
            printf("%s %s %d %f %d\n", sfx_file.id.c_str(), sfx_file.path.c_str(), sfx_file.variations,
                sfx_file.pitch_variance, sfx_file.max_instances);
        }
        for (GfxFrame &gfx_frame : pack.gfx_frames) {
            if (!gfx_frame.id.size()) continue;
            printf("%s %s %u %u %u %u\n", gfx_frame.id.c_str(), gfx_frame.gfx.c_str(),
                gfx_frame.x, gfx_frame.y, gfx_frame.w, gfx_frame.h);
        }
        for (GfxAnim &gfx_anim : pack.gfx_anims) {
            if (!gfx_anim.id.size()) continue;
            printf("%s %s %u %u %u", gfx_anim.id.c_str(), gfx_anim.sound.c_str(),
                gfx_anim.frame_rate, gfx_anim.frame_count, gfx_anim.frame_delay);
            for (int i = 0; i < gfx_anim.frames.size(); i++) {
                printf(" %s", gfx_anim.frames[i].c_str());
            }
            printf("\n");
        }
        for (Material material : pack.materials) {
            if (!material.id.size()) continue;
            printf("%-20s %-20s walk=%d swim=%d\n", material.id.c_str(), material.footstep_sound.c_str(),
                (bool)(material.flags & MATERIAL_FLAG_WALK),
                (bool)(material.flags & MATERIAL_FLAG_SWIM)
            );
        }
        for (Sprite &sprite : pack.sprites) {
            if (!sprite.id.size()) continue;
            printf("%s %s %s %s %s %s %s %s %s\n",
                sprite.id.c_str(),
                sprite.anims[0].c_str(),
                sprite.anims[1].c_str(),
                sprite.anims[2].c_str(),
                sprite.anims[3].c_str(),
                sprite.anims[4].c_str(),
                sprite.anims[5].c_str(),
                sprite.anims[6].c_str(),
                sprite.anims[7].c_str()
            );
        }
    }
#endif

    void CompressFile(const char *src, const char *dst)
    {
        DatBuffer raw{};
        int bytesRead = 0;
        raw.bytes = LoadFileData(src, &bytesRead);
        raw.length = bytesRead;

        DatBuffer compressed{};
        int compressedSize = 0;
        compressed.bytes = CompressData(raw.bytes, raw.length, &compressedSize);
        compressed.length = compressedSize;

        SaveFileData(dst, compressed.bytes, compressed.length);

        MemFree(raw.bytes);
        MemFree(compressed.bytes);
    }

    Err LoadResources(Pack &pack);

    void Init(void)
    {
        Err err = RN_SUCCESS;

        GenPlaceholderTexture();

#if DEV_BUILD_PACK_FILE
        Pack packHardcoded{ "pack/pack1.dat" };
        PackAddMeta(packHardcoded, "resources/map/map_overworld.mdesk");
        PackAddMeta(packHardcoded, "resources/map/map_cave.mdesk");
        PackAddMeta(packHardcoded, "resources/meta/overworld.mdesk");
        //PackDebugPrint(packHardcoded);

        //LoadResources(packHardcoded);
        err = SavePack(packHardcoded, PACK_TYPE_BINARY);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to save data file.\n");
        }
#endif

        Pack *pack1 = new Pack{ "pack/pack1.dat" };
        packs.push_back(pack1);

        err = LoadPack(*pack1, PACK_TYPE_BINARY);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to load data file.\n");
        }

#if 0
        // Doesn't really work.. probably don't need it tbh
        packHardcoded.path = "pack/pack1.txt";
        err = SavePack(packHardcoded, PACK_TYPE_TEXT);
        if (err) {
            assert(!err);
            TraceLog(LOG_ERROR, "Failed to save data file as text.\n");
        }
#endif

#if 0
        CompressFile("pack/pack1.dat", "pack/pack1.smol");
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

    template<class T>
    void Process(PackStream &stream, T &v)
    {
        static_assert(std::is_fundamental_v<T> || std::is_enum_v<T>,
            "You cannot fread/write a non-primitive type");

        if (stream.type == PACK_TYPE_BINARY) {
            stream.process(&v, sizeof(v), 1, stream.file);
        } else {
            // text mode not implemented
            assert("nope");
        }

        if (stream.mode == PACK_MODE_WRITE) {
            fflush(stream.file);
        }
    }
    void Process(PackStream &stream, std::string &str)
    {
        uint16_t strLen = (uint16_t)str.size();
        PROC(strLen);
        str.resize(strLen);
        for (int i = 0; i < strLen; i++) {
            PROC(str[i]);
        }
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
    void Process(PackStream &stream, Rectangle &rec)
    {
        PROC(rec.x);
        PROC(rec.y);
        PROC(rec.width);
        PROC(rec.height);
    }
    void Process(PackStream &stream, DatBuffer &buffer)
    {
        PROC(buffer.length);
        if (buffer.length) {
            if (!buffer.bytes) {
                assert(stream.mode == PACK_MODE_READ);
                buffer.bytes = (uint8_t *)MemAlloc(buffer.length);
            }
            for (size_t i = 0; i < buffer.length; i++) {
                PROC(buffer.bytes[i]);
            }
        }
    }

    void Process(PackStream &stream, GfxFile &gfx_file, int index)
    {
        PROC(gfx_file.id);
        PROC(gfx_file.path);
        PROC(gfx_file.data_buffer);
        if (stream.mode == PACK_MODE_READ && !gfx_file.data_buffer.length && !gfx_file.path.empty()) {
            ReadFileIntoDataBuffer(gfx_file.path.c_str(), gfx_file.data_buffer);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->gfx_file_by_id[gfx_file.id] = index;
        }
    }
    void Process(PackStream &stream, MusFile &mus_file, int index)
    {
        PROC(mus_file.id);
        PROC(mus_file.path);
        PROC(mus_file.data_buffer);
        if (stream.mode == PACK_MODE_READ && !mus_file.data_buffer.length && !mus_file.path.empty()) {
            // TODO(dlb): Music is streaming, don't read whole file into memory
            ReadFileIntoDataBuffer(mus_file.path.c_str(), mus_file.data_buffer);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->mus_file_by_id[mus_file.id] = index;
        }
    }
    void Process(PackStream &stream, SfxFile &sfx_file, int index)
    {
        PROC(sfx_file.id);
        PROC(sfx_file.path);
        PROC(sfx_file.data_buffer);
        PROC(sfx_file.variations);
        PROC(sfx_file.pitch_variance);
        PROC(sfx_file.max_instances);

        if (stream.mode == PACK_MODE_READ && !sfx_file.data_buffer.length && !sfx_file.path.empty()) {
            ReadFileIntoDataBuffer(sfx_file.path.c_str(), sfx_file.data_buffer);
        }

        if (stream.mode == PACK_MODE_READ) {
            auto &sfx_variants = stream.pack->sfx_file_by_id[sfx_file.id];
            sfx_variants.push_back(index);
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

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->gfx_frame_by_id[gfx_frame.id] = index;
        }
    }
    void Process(PackStream &stream, GfxAnim &gfx_anim, int index)
    {
        PROC(gfx_anim.id);
        PROC(gfx_anim.sound);
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
        PROC(material.footstep_sound);
        PROC(material.flags);

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->material_by_id[material.id] = index;
        }
    }
    void Process(PackStream &stream, Object &object, int index) {
        PROC(object.id);
        PROC(object.type);
        PROC(object.animation);

        if (object.type == "lootable") {
            PROC(object.loot_table_id);
        } else if (object.type == "warp") {
            PROC(object.warp_map_id);
            PROC(object.warp_dest_pos);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->object_by_id[object.id] = index;
        }
    }
    void Process(PackStream &stream, Sprite &sprite, int index) {
        PROC(sprite.id);
        assert(ARRAY_SIZE(sprite.anims) == 8); // if this changes, version must increment
        for (int i = 0; i < 8; i++) {
            PROC(sprite.anims[i]);
        }

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->sprite_by_id[sprite.id] = index;
        }
    }
    void Process(PackStream &stream, TileDef &tile_def, int index) {
        PROC(tile_def.id);
        PROC(tile_def.anim);
        PROC(tile_def.material);
        PROC(tile_def.auto_tile_mask);

        // TODO: Idk where/how to do this, but we don't have the texture
        //       loaded yet in this context, necessarily.
        //tileDef.color = GetImageColor(texEntry.image, tileDef.x, tileDef.y);

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->tile_def_by_id[tile_def.id] = index;
        }
    }

    void Process(PackStream &stream, Entity &entity, int index)
    {
        bool alive = entity.id && !entity.despawned_at && entity.type;
        PROC(alive);
        if (!alive) {
            return;
        }

        //// Entity ////
        PROC(entity.id);
        PROC((uint8_t &)entity.type);
        PROC((uint8_t &)entity.spec);
        PROC(entity.name);
        PROC(entity.caused_by);
        PROC(entity.spawned_at);
        //PROC(entity.despawned_at);

        PROC(entity.map_id);
        PROC(entity.position);

        PROC(entity.ambient_fx);
        PROC(entity.ambient_fx_delay_min);
        PROC(entity.ambient_fx_delay_max);

        PROC(entity.radius);
        //PROC(entity.colliding);
        //PROC(entity.on_warp);

        //PROC(entity.last_attacked_at);
        //PROC(entity.attack_cooldown);

        PROC(entity.dialog_root_key);
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
        PROC((uint8_t &)entity.direction);
        //PROC(entity.anim_frame);
        //PROC(entity.anim_accum);

        PROC(entity.warp_collider);
        PROC(entity.warp_dest_pos);

        PROC(entity.warp_dest_map);
        PROC(entity.warp_template_map);
        PROC(entity.warp_template_tileset);

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
    void Process(PackStream &stream, Tilemap &tile_map, int index)
    {
        uint32_t sentinel = 0;
        if (stream.mode == PackStreamMode::PACK_MODE_WRITE) {
            sentinel = Tilemap::SENTINEL;
        }

        if (stream.mode == PackStreamMode::PACK_MODE_WRITE) {
            assert(tile_map.width * tile_map.height == tile_map.tiles.size());
            assert(tile_map.width * tile_map.height == tile_map.objects.size());
        }

        PROC(tile_map.version);
        PROC(tile_map.id);
        PROC(tile_map.width);
        PROC(tile_map.height);

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        //SetTextureWrap(texEntry.texture, TEXTURE_WRAP_CLAMP);

        //if (!width || !height) {
        //    err = RN_INVALID_SIZE; break;
        //}

        uint32_t tileDefCount   = tile_map.tileDefs.size();
        uint32_t objectDefCount = tile_map.objectDefs.size();
        uint32_t pathNodeCount  = tile_map.pathNodes.size();
        uint32_t pathCount      = tile_map.paths.size();

        PROC(tileDefCount);
        PROC(objectDefCount);
        PROC(pathNodeCount);
        PROC(pathCount);

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        tile_map.tileDefs  .resize(tileDefCount);
        tile_map.objectDefs.resize(objectDefCount);
        tile_map.tiles     .resize((size_t)tile_map.width * tile_map.height);
        tile_map.objects   .resize((size_t)tile_map.width * tile_map.height);
        tile_map.pathNodes .resize(pathNodeCount);
        tile_map.paths     .resize(pathCount);

        for (std::string &tile_def_id : tile_map.tileDefs) {
            PROC(tile_def_id);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        for (std::string &object_def_id : tile_map.objectDefs) {
            PROC(object_def_id);
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

        for (uint8_t &object : tile_map.objects) {
            PROC(object);
            if (object >= objectDefCount) {
                object = 0;
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

        for (data::AiPath &aiPath : tile_map.paths) {
            PROC(aiPath.pathNodeStart);
            PROC(aiPath.pathNodeCount);
        }

        PROC(sentinel);
        assert(sentinel == Tilemap::SENTINEL);

        if (stream.mode == PACK_MODE_READ) {
            stream.pack->tile_map_by_id[tile_map.id] = index;
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

        int tocOffsetPos = ftell(stream.file);
        pack.toc_offset = 0;
        PROC(pack.toc_offset);

        if (stream.mode == PACK_MODE_WRITE) {
            #define WRITE_ARRAY(arr) \
                /* 0 slot is reserved, skip it when writing */\
                for (int i = 1; i < arr.size(); i++) { \
                    auto &entry = arr[i]; \
                    pack.toc.entries.push_back(PackTocEntry(entry.dtype, ftell(stream.file))); \
                    Process(stream, entry, i); \
                }

            WRITE_ARRAY(pack.gfx_files);
            WRITE_ARRAY(pack.mus_files);
            WRITE_ARRAY(pack.sfx_files);
            WRITE_ARRAY(pack.gfx_frames);
            WRITE_ARRAY(pack.gfx_anims);
            WRITE_ARRAY(pack.materials);
            WRITE_ARRAY(pack.objects);
            WRITE_ARRAY(pack.sprites);
            WRITE_ARRAY(pack.tile_defs);
            WRITE_ARRAY(pack.tile_maps);
            WRITE_ARRAY(pack.entities);

            #undef WRITE_ARRAY

            int tocOffset = ftell(stream.file);
            int entryCount = (int)pack.toc.entries.size();
            PROC(entryCount);
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC((uint8_t &)tocEntry.dtype);
                PROC(tocEntry.offset);
            }

            fseek(stream.file, tocOffsetPos, SEEK_SET);
            PROC(tocOffset);
        } else {
            fseek(stream.file, pack.toc_offset, SEEK_SET);
            int entryCount = 0;
            PROC(entryCount);
            pack.toc.entries.resize(entryCount);

            int typeCounts[DAT_TYP_COUNT]{};
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC((uint8_t &)tocEntry.dtype);
                PROC(tocEntry.offset);
                typeCounts[tocEntry.dtype]++;
            }

            pack.gfx_files .resize((size_t)1 + typeCounts[DAT_TYP_GFX_FILE]);
            pack.mus_files .resize((size_t)1 + typeCounts[DAT_TYP_MUS_FILE]);
            pack.sfx_files .resize((size_t)1 + typeCounts[DAT_TYP_SFX_FILE]);
            pack.gfx_frames.resize((size_t)1 + typeCounts[DAT_TYP_GFX_FRAME]);
            pack.gfx_anims .resize((size_t)1 + typeCounts[DAT_TYP_GFX_ANIM]);
            pack.materials .resize((size_t)1 + typeCounts[DAT_TYP_MATERIAL]);
            pack.objects   .resize((size_t)1 + typeCounts[DAT_TYP_OBJECT]);
            pack.sprites   .resize((size_t)1 + typeCounts[DAT_TYP_SPRITE]);
            pack.tile_defs .resize((size_t)1 + typeCounts[DAT_TYP_TILE_DEF]);
            pack.tile_maps .resize((size_t)1 + typeCounts[DAT_TYP_TILE_MAP]);
            pack.entities  .resize((size_t)1 + typeCounts[DAT_TYP_ENTITY]);

            int typeNextIndex[DAT_TYP_COUNT]{};
            // 0 slot is reserved, skip it when reading
            for (int i = 0; i < DAT_TYP_COUNT; i++) {
                typeNextIndex[i] = 1;
            }

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.file, tocEntry.offset, SEEK_SET);
                int index = typeNextIndex[tocEntry.dtype];
                tocEntry.index = index;
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  Process(stream, pack.gfx_files [index], index); break;
                    case DAT_TYP_MUS_FILE:  Process(stream, pack.mus_files [index], index); break;
                    case DAT_TYP_SFX_FILE:  Process(stream, pack.sfx_files [index], index); break;
                    case DAT_TYP_GFX_FRAME: Process(stream, pack.gfx_frames[index], index); break;
                    case DAT_TYP_GFX_ANIM:  Process(stream, pack.gfx_anims [index], index); break;
                    case DAT_TYP_MATERIAL:  Process(stream, pack.materials [index], index); break;
                    case DAT_TYP_OBJECT:    Process(stream, pack.objects   [index], index); break;
                    case DAT_TYP_SPRITE:    Process(stream, pack.sprites   [index], index); break;
                    case DAT_TYP_TILE_DEF:  Process(stream, pack.tile_defs [index], index); break;
                    case DAT_TYP_TILE_MAP:  Process(stream, pack.tile_maps [index], index); break;
                    case DAT_TYP_ENTITY:    Process(stream, pack.entities  [index], index); break;
                }
                typeNextIndex[tocEntry.dtype]++;
            }
        }

        return err;
    }
#undef PROC

    Err SavePack(Pack &pack, PackStreamType type)
    {
        Err err = RN_SUCCESS;
#if 0
        if (FileExists(pack.path.c_str())) {
            err = MakeBackup(pack.path.c_str());
            if (err) return err;
        }
#endif
        FILE *file = fopen(pack.path.c_str(), type == PACK_TYPE_BINARY ? "wb" : "w");
        if (!file) {
            return RN_BAD_FILE_WRITE;
        }

        PackStream stream{ &pack, file, PACK_MODE_WRITE, type };
        err = Process(stream);

        fclose(stream.file);
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
    Err LoadPack(Pack &pack, PackStreamType type)
    {
        Err err = RN_SUCCESS;

        FILE *file = fopen(pack.path.c_str(), type == PACK_TYPE_BINARY ? "rb" : "r");
        if (!file) {
            return RN_BAD_FILE_READ;
        }

        PackStream stream{ &pack, file, PACK_MODE_READ, type };
        do {
            err = Process(stream);
            if (err) break;

            err = LoadResources(*stream.pack);
            if (err) break;
        } while(0);

        if (stream.file) fclose(stream.file);
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

    void PlaySound(const std::string &id, float pitchVariance)
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
    bool IsSoundPlaying(const std::string &id)
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
    void StopSound(const std::string &id)
    {
        const SfxFile &sfx_file = packs[0]->FindSoundVariant(id);
        assert(sfx_file.instances.size() == sfx_file.max_instances);

        for (int i = 0; i < sfx_file.max_instances; i++) {
            StopSound(sfx_file.instances[i]);
        }
    }

    void UpdateGfxAnim(const GfxAnim &anim, double dt, GfxAnimState &anim_state)
    {
        anim_state.accum += dt;

        const double frame_time = (1.0 / anim.frame_rate) * anim.frame_delay;
        if (anim_state.accum >= frame_time) {
            anim_state.frame++;
            if (anim_state.frame >= anim.frame_count) {
                anim_state.frame = 0;
            }
            anim_state.accum -= frame_time;
        }
    }

    const GfxFrame &GetSpriteFrame(const Entity &entity)
    {
        const Sprite &sprite = packs[0]->FindSprite(entity.sprite);

        const std::string &anim_id = sprite.anims[entity.direction];
        const GfxAnim &anim = packs[0]->FindGraphicAnim(anim_id);

        const std::string *frame_id = 0;
        if (!anim.frames.empty()) {
            frame_id = &anim.frames[entity.anim_state.frame];
        }
        const GfxFrame &frame = packs[0]->FindGraphicFrame(frame_id ? *frame_id : "");

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

        const Sprite &sprite = packs[0]->FindSprite(entity.sprite);
        const GfxAnim &anim = packs[0]->FindGraphicAnim(sprite.anims[entity.direction]);
        UpdateGfxAnim(anim, dt, entity.anim_state);

        if (newlySpawned) {
            const SfxFile &sfx_file = packs[0]->FindSoundVariant(anim.sound);
            PlaySound(sfx_file.id);
        }
    }
    void ResetSprite(Entity &entity)
    {
        const Sprite &sprite = packs[0]->FindSprite(entity.sprite);
        GfxAnim &anim = packs[0]->FindGraphicAnim(sprite.anims[entity.direction]);
        StopSound(anim.sound);

        entity.anim_state.frame = 0;
        entity.anim_state.accum = 0;
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

    void UpdateTileDefAnimations(double dt)
    {
        for (TileDef &tile_def : data::packs[0]->tile_defs) {
            const GfxAnim &anim = packs[0]->FindGraphicAnim(tile_def.anim);
            data::UpdateGfxAnim(anim, dt, tile_def.anim_state);
        }
    }

#define ENUM_STR_GENERATOR(type, enumDef, enumGen) \
    const char *type##Str(type id) {               \
        switch (id) {                              \
            enumDef(enumGen)                       \
            default: return "<unknown>";           \
        }                                          \
    }
    ENUM_STR_GENERATOR(DataType,      DATA_TYPES,     ENUM_GEN_CASE_RETURN_DESC);
    ENUM_STR_GENERATOR(EntityType,    ENTITY_TYPES,   ENUM_GEN_CASE_RETURN_STR);
    ENUM_STR_GENERATOR(EntitySpecies, ENTITY_SPECIES, ENUM_GEN_CASE_RETURN_STR);
#undef ENUM_STR_GENERATOR
}