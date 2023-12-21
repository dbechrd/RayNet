#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "perf_timer.h"
#include "schema.h"

#define MD_HIJACK_STRING_CONSTANTS 0
#include "md.h"

#include "capnp/message.h"
#include "capnp/serialize.h"
#include "capnp/serialize-text.h"
#include "meta/resource_library.capnp.h"

#include <io.h>
#include <fcntl.h>
//#include <iomanip>
//#include <sstream>
//#include <type_traits>

struct GameState {
    bool freye_introduced;
};
GameState game_state;

std::vector<Pack> packs;

Shader shdSdfText;
Shader shdPixelFixer;
int    shdPixelFixerScreenSizeUniformLoc;

Font fntTiny;
Font fntSmall;
Font fntMedium;
Font fntBig;

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

        // assets.txt
        // assets.txt -> assets.bin
        // assets.bin -> C++ objects
        // C++ objects -> assets.bin
        // assets.bin -> asset.txt

        fprintf(file, "@Tilemap %u: {\n", tilemap.id);
        fprintf(file, "    version: %u\n", tilemap.version);
        fprintf(file, "    name: %s\n", tilemap.name.c_str());
        fprintf(file, "    width: %u\n", tilemap.width);
        fprintf(file, "    height: %u\n", tilemap.height);
        fprintf(file, "    title: \"%s\"\n", tilemap.title.c_str());
        fprintf(file, "    background_music: %s\n", tilemap.background_music.c_str());
        fprintf(file, "    ground_tiles: [");
        for (int i = 0; i < tilemap.tiles.size(); i++) {
            if (i % tilemap.width == 0) {
                fprintf(file, "\n        ");
            }
            fprintf(file, "%3u", tilemap.tiles[i]);
        }
        fprintf(file, "\n");
        fprintf(file, "    ]\n");

        fprintf(file, "    objects: [");
        for (int i = 0; i < tilemap.objects.size(); i++) {
            if (i % tilemap.width == 0) {
                fprintf(file, "\n        ");
            }
            fprintf(file, "%3u", tilemap.objects[i]);
        }
        fprintf(file, "\n");
        fprintf(file, "    ]\n");

        fprintf(file, "    object_data: [\n");
        fprintf(file, "        //x y type [type_data]\n");
        fprintf(file, "        //    lever    powered tile_def_unpowered tile_def_powered\n");
        fprintf(file, "        //    lootable loot_table_id\n");
        fprintf(file, "        //    sign     \"line 1\" \"line 2\" \"line 3\" \"line 4\"\n");
        fprintf(file, "        //    warp     dest_map_id dest_x dest_y dest_z\n");
        for (ObjectData &obj_data : tilemap.object_data) {
            fprintf(file, "        { %u %u %s ",
                obj_data.x,
                obj_data.y,
                obj_data.type.c_str()
            );
            if (obj_data.type == "lever") {
                fprintf(file, "%u ", obj_data.power_level);
                fprintf(file, "%u ", obj_data.tile_def_unpowered);
                fprintf(file, "%u ", obj_data.tile_def_powered);
            } else if (obj_data.type == "lootable") {
                fprintf(file, "%u ", obj_data.loot_table_id);
            } else if (obj_data.type == "sign") {
                fprintf(file, "\"%s\" ", obj_data.sign_text[0].c_str());
                fprintf(file, "\"%s\" ", obj_data.sign_text[1].c_str());
                fprintf(file, "\"%s\" ", obj_data.sign_text[2].c_str());
                fprintf(file, "\"%s\" ", obj_data.sign_text[3].c_str());
            } else if (obj_data.type == "warp") {
                fprintf(file, "%u %u %u %u ",
                    obj_data.warp_map_id,
                    obj_data.warp_dest_x,
                    obj_data.warp_dest_y,
                    obj_data.warp_dest_z
                );
            }
            fprintf(file, "}\n");
        }
        fprintf(file, "    ]\n");

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
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Identifier) {
        if (result) {
            *result = std::string((char *)node->string.str, node->string.size);
        }
        return true;
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected identifier."));
        return false;
    }
}
bool dlb_MD_Expect_String(MD_Node *node, std::string *result)
{
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_StringLiteral) {
        if (result) {
            *result = std::string((char *)node->string.str, node->string.size);
        }
        return true;
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected double-quoted string."));
        return false;
    }
}
bool dlb_MD_Expect_Int(MD_Node *node, int min, int max, int *result)
{
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric && MD_StringIsCStyleInt(node->string)) {
        int val = (int)MD_CStyleIntFromString(node->string);
        if (val >= min && val <= max) {
            if (result) {
                *result = val;
            }
            return true;
        } else {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected integer in inclusive range [%d, %d].", min, max);
            return false;
        }
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected integer literal."));
        return false;
    }

}
bool dlb_MD_Expect_Uint_internal(MD_Node *node, uint32_t min, uint32_t max, uint32_t *result)
{
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric && MD_StringIsCStyleInt(node->string)) {
        MD_u64 val = MD_U64FromString(node->string, 10);
        if (val >= min && val <= max) {
            if (result) {
                *result = val;
            }
            return true;
        } else {
            MD_CodeLoc loc = MD_CodeLocFromNode(node);
            MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected unsigned integer in inclusive range [%u, %u].", min, max);
            return false;
        }
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected unsigned integer literal."));
        return false;
    }
}
bool dlb_MD_Expect_Uint8(MD_Node *node, uint8_t *result)
{
    uint32_t val = 0;
    if (dlb_MD_Expect_Uint_internal(node, 0, UINT8_MAX, &val)) {
        *result = (uint8_t)val;
        return true;
    }
    return false;
}
bool dlb_MD_Expect_Uint16(MD_Node *node, uint16_t *result)
{
    uint32_t val = 0;
    if (dlb_MD_Expect_Uint_internal(node, 0, UINT16_MAX, &val)) {
        *result = (uint16_t)val;
        return true;
    }
    return false;
}
bool dlb_MD_Expect_Uint32(MD_Node *node, uint32_t *result)
{
    uint32_t val = 0;
    if (dlb_MD_Expect_Uint_internal(node, 0, UINT32_MAX, &val)) {
        *result = (uint32_t)val;
        return true;
    }
    return false;
}
bool dlb_MD_Expect_Float(MD_Node *node, float *result)
{
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric) {
        if (result) {
            *result = (float)MD_F64FromString(node->string);
        }
        return true;
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected float literal."));
        return false;
    }
}
bool dlb_MD_Expect_Double(MD_Node *node, double *result)
{
    if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric) {
        if (result) {
            *result = MD_F64FromString(node->string);
        }
        return true;
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessage(stderr, loc, MD_MessageKind_Error, MD_S8Lit("Expected double literal."));
        return false;
    }
}
bool dlb_MD_Expect_Nil(MD_Node *node)
{
    if (MD_NodeIsNil(node)) {
        return true;
    } else {
        MD_CodeLoc loc = MD_CodeLocFromNode(node);
        MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, (char *)"Expected end of scope.");
        return false;
    }
}

#define META_ID_STR(dest) \
if (!dlb_MD_Expect_Ident(node, &dest)) break;
#define META_ID_UINT32(dest) \
if (!dlb_MD_Expect_Uint32(node, &dest)) break;
#define META_IDENT(dest) \
    if (!dlb_MD_Expect_Ident(node, &dest)) break; \
    node = node->next;
#define META_STRING(dest) \
    if (!dlb_MD_Expect_String(node, &dest)) break; \
    node = node->next;
#define META_UINT8(dest) \
    if (!dlb_MD_Expect_Uint8(node, &dest)) break; \
    node = node->next;
#define META_UINT16(dest) \
    if (!dlb_MD_Expect_Uint16(node, &dest)) break; \
    node = node->next;
#define META_UINT32(dest) \
    if (!dlb_MD_Expect_Uint32(node, &dest)) break; \
    node = node->next;
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

                META_ID_STR(gfx_file.id);
                META_CHILDREN_BEGIN;
                    META_STRING(gfx_file.path);
                META_CHILDREN_END;

                pack.gfx_files.push_back(gfx_file);
            } else if (MD_NodeHasTag(node, MD_S8Lit("MusFile"), 0)) {
                MusFile mus_file{};

                META_ID_STR(mus_file.id);
                META_CHILDREN_BEGIN;
                    META_STRING(mus_file.path);
                META_CHILDREN_END;

                pack.mus_files.push_back(mus_file);
            } else if (MD_NodeHasTag(node, MD_S8Lit("SfxFile"), 0)) {
                SfxFile sfx_file{};

                META_ID_STR(sfx_file.id);
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

                META_ID_STR(gfx_frame.id);
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

                META_ID_STR(gfx_anim.id);
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
            } else if (MD_NodeHasTag(node, MD_S8Lit("Sprite"), 0)) {
                Sprite sprite{};

                META_ID_UINT32(sprite.id);
                META_CHILDREN_BEGIN;
                    META_STRING(sprite.name);
                    for (int i = 0; i < ARRAY_SIZE(sprite.anims); i++) {
                        META_IDENT(sprite.anims[i]);
                    }
                META_CHILDREN_END;

                pack.sprites.push_back(sprite);
            } else if (MD_NodeHasTag(node, MD_S8Lit("TileDef"), 0)) {
                TileDef tile_def{};

                META_ID_UINT32(tile_def.id);
                META_CHILDREN_BEGIN;
                    META_STRING(tile_def.name);
                    META_IDENT(tile_def.anim);
                    META_UINT32(tile_def.material_id);
                    uint8_t flag_solid = 0;
                    META_UINT8(flag_solid);
                    uint8_t flag_liquid = 0;
                    META_UINT8(flag_liquid);
                    if (flag_solid)  tile_def.flags |= TILEDEF_FLAG_SOLID;
                    if (flag_liquid) tile_def.flags |= TILEDEF_FLAG_LIQUID;
                    META_UINT8(tile_def.auto_tile_mask);
                META_CHILDREN_END;

                pack.tile_defs.push_back(tile_def);
            } else if (MD_NodeHasTag(node, MD_S8Lit("TileMat"), 0)) {
                TileMat tile_mat{};

                META_ID_UINT32(tile_mat.id);
                META_CHILDREN_BEGIN;
                    META_STRING(tile_mat.name);
                    META_IDENT(tile_mat.footstep_sound);
                META_CHILDREN_END;

                pack.tile_mats.push_back(tile_mat);
            } else if (MD_NodeHasTag(node, MD_S8Lit("Tilemap"), 0)) {
                Tilemap map{};

                META_ID_UINT32(map.id);
                META_CHILDREN_BEGIN;           // {}
                    META_CHILDREN_LOOP_BEGIN;  // key: value
                        std::string key{};
                        META_ID_STR(key);      // key

                        META_CHILDREN_BEGIN;   // value
                        if (key == "name") {
                            META_STRING(map.name);
                        } else if (key == "version") {
                            META_UINT32(map.version);
                        } else if (key == "width") {
                            META_UINT32(map.width);
                        } else if (key == "height") {
                            META_UINT32(map.height);
                        } else if (key == "title") {
                            META_STRING(map.title);
                        } else if (key == "background_music") {
                            META_IDENT(map.background_music);
                        } else if (key == "ground_tiles") {
                            META_CHILDREN_LOOP_BEGIN;  // []
                                uint32_t tile_id{};
                                META_UINT32(tile_id);
                                map.tiles.push_back(tile_id);
                            META_CHILDREN_LOOP_END;
                        } else if (key == "objects") {
                            META_CHILDREN_LOOP_BEGIN;  // []
                                uint32_t object{};
                                META_UINT32(object);
                                map.objects.push_back(object);
                            META_CHILDREN_LOOP_END;
                        } else if (key == "object_data") {
                            META_CHILDREN_LOOP_BEGIN;  // []
                                ObjectData obj_data{};

                                META_CHILDREN_BEGIN;  // {}
                                    META_UINT32(obj_data.x);
                                    META_UINT32(obj_data.y);
                                    META_IDENT(obj_data.type);
                                    if (obj_data.type == "lever") {
                                        META_UINT8(obj_data.power_level);
                                        META_UINT32(obj_data.tile_def_unpowered);
                                        META_UINT32(obj_data.tile_def_powered);
                                    } else if (obj_data.type == "lootable") {
                                        META_UINT32(obj_data.loot_table_id);
                                    } else if (obj_data.type == "sign") {
                                        META_STRING(obj_data.sign_text[0]);
                                        META_STRING(obj_data.sign_text[1]);
                                        META_STRING(obj_data.sign_text[2]);
                                        META_STRING(obj_data.sign_text[3]);
                                    } else if (obj_data.type == "warp") {
                                        std::string warp_map_name{};
                                        META_UINT32(obj_data.warp_map_id);
                                        META_UINT32(obj_data.warp_dest_x);
                                        META_UINT32(obj_data.warp_dest_y);
                                        META_UINT32(obj_data.warp_dest_z);
                                    }
                                META_CHILDREN_END;

                                map.object_data.push_back(obj_data);
                            META_CHILDREN_LOOP_END_NEXT;
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

void CompressFile(const char *srcFileName, const char *dstFileName)
{
    DatBuffer raw{};
    int bytesRead = 0;
    raw.bytes = LoadFileData(srcFileName, &bytesRead);
    raw.length = bytesRead;

    DatBuffer compressed{};
    int compressedSize = 0;
    compressed.bytes = CompressData(raw.bytes, raw.length, &compressedSize);
    compressed.length = compressedSize;

    SaveFileData(dstFileName, compressed.bytes, compressed.length);

    MemFree(raw.bytes);
    MemFree(compressed.bytes);
}

Err LoadResources(Pack &pack);

Err Init(void)
{
    {
        capnp::MallocMessageBuilder message;

        meta::ResourceLibrary::Builder resourceLibrary = message.initRoot<meta::ResourceLibrary>();
        capnp::List<meta::Resource>::Builder resources = resourceLibrary.initResources(2);

        meta::Resource::Builder gfx_missing = resources[0];
        gfx_missing.setId(0);
        gfx_missing.setName("gfx_missing");
        gfx_missing.setPath("resources/graphics/missing.png");

        meta::Resource::Builder gfx_dlg_npatch = resources[1];
        gfx_dlg_npatch.setId(1);
        gfx_dlg_npatch.setName("gfx_dlg_npatch");
        gfx_dlg_npatch.setPath("resources/graphics/npatch.png");

        int fd = _open("capnp/resource_library.bin", _O_RDWR | _O_CREAT | _O_BINARY, _S_IWRITE);
        capnp::writeMessageToFd((int)fd, message);
        _close(fd);
    }

    //NewEnt::run_tests();

#if 0
    {
        ta_schema_register();
        foo dat{};
        ta_schema_print(stdout, TYP_FOO, &dat, 0, 0);
        //ta_schema_print_json(stdout, TYP_FOO, &dat, 0, 0);
    }
#endif

    PerfTimer t{ "InitCommon" };

    Err err = RN_SUCCESS;

    rnStringCatalog.Init();

    // LoadPack SDF required shader (we use default vertex shader)
    shdSdfText = LoadShader(0, "resources/shader/sdf.fs");

    shdPixelFixer                     = LoadShader("resources/shader/pixelfixer.vs", "resources/shader/pixelfixer.fs");
    shdPixelFixerScreenSizeUniformLoc = GetShaderLocation(shdPixelFixer, "screenSize");

#if 0
    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    if (!FileExists(fontName)) {
        fontName = "resources/font/KarminaBold.otf";
    }
#else
    const char *fontName = "resources/font/KarminaBold.otf";
    //const char *fontName = "resources/font/PixelOperator-Bold.ttf";
    //const char *fontName = "resources/font/PixelOperatorMono-Bold.ttf";
#endif

    fntTiny = dlb_LoadFontEx(fontName, 14, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntTiny.baseSize, RN_RAYLIB_ERROR);

    fntSmall = dlb_LoadFontEx(fontName, 18, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntSmall.baseSize, RN_RAYLIB_ERROR);

    fntMedium = dlb_LoadFontEx(fontName, 20, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntMedium.baseSize, RN_RAYLIB_ERROR);

    fntBig = dlb_LoadFontEx(fontName, 46, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntBig.baseSize, RN_RAYLIB_ERROR);
    //SetTextureFilter(fntBig.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    //GenPlaceholderTexture();

#if DEV_BUILD_PACK_FILE
    {
        PerfTimer t{ "Build pack files" };
        Pack packAssets{ "pack/assets.dat" };
        Pack packMaps{ "pack/maps.dat" };

        {
            PerfTimer t{ "Parse mdesk" };
            PackAddMeta(packAssets, "resources/meta/overworld.mdesk");
            PackAddMeta(packMaps, "resources/map/map_overworld.mdesk");
            PackAddMeta(packMaps, "resources/map/map_cave.mdesk");
        }

        //PackDebugPrint(packAssets);
        //PackDebugPrint(packMaps);

        // If you want the big buffers in your pack, call this!
        //LoadResources(packAssets);
        //LoadResources(packMaps);

        ERR_RETURN(SavePack(packAssets, PACK_TYPE_BINARY));
        ERR_RETURN(SavePack(packMaps, PACK_TYPE_BINARY));

        packAssets.path = "pack/assets.txt";
        ERR_RETURN(SavePack(packAssets, PACK_TYPE_TEXT));
    }
#endif

    packs.emplace_back("pack/assets.dat");
    packs.emplace_back("pack/maps.dat");

    for (Pack &pack : packs) {
        ERR_RETURN(LoadPack(pack, PACK_TYPE_BINARY));
    }

#if 0
    CompressFile("pack/assets.dat", "pack/assets.smol");
    CompressFile("pack/maps.dat", "pack/maps.smol");
#endif

    // TODO: Pack these too
    LoadDialogFile("resources/dialog/lily.md");
    LoadDialogFile("resources/dialog/freye.md");
    LoadDialogFile("resources/dialog/nessa.md");
    LoadDialogFile("resources/dialog/elane.md");

    dialog_library.RegisterListener("DIALOG_FREYE_INTRO", FreyeIntroListener);

    return err;
}
void Free(void)
{
    UnloadShader(shdSdfText);
    UnloadShader(shdPixelFixer);
    UnloadFont(fntTiny);
    UnloadFont(fntSmall);
    UnloadFont(fntMedium);
    UnloadFont(fntBig);

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
    } else if (stream.type == PACK_TYPE_TEXT) {
        if (stream.mode == PACK_MODE_WRITE) {
            if      constexpr (std::is_same_v<T, char    >) fprintf(stream.file, "%c", v);
            else if constexpr (std::is_same_v<T, int8_t  >) fprintf(stream.file, "%" PRId8 , v);
            else if constexpr (std::is_same_v<T, int16_t >) fprintf(stream.file, "%" PRId16, v);
            else if constexpr (std::is_same_v<T, int32_t >) fprintf(stream.file, "%" PRId32, v);
            else if constexpr (std::is_same_v<T, int64_t >) fprintf(stream.file, "%" PRId64, v);
            else if constexpr (std::is_same_v<T, uint8_t >) fprintf(stream.file, "%" PRIu8 , v);
            else if constexpr (std::is_same_v<T, uint16_t>) fprintf(stream.file, "%" PRIu16, v);
            else if constexpr (std::is_same_v<T, uint32_t>) fprintf(stream.file, "%" PRIu32, v);
            else if constexpr (std::is_same_v<T, uint64_t>) fprintf(stream.file, "%" PRIu64, v);
            else if constexpr (std::is_same_v<T, float   >) fprintf(stream.file, "%f", v);
            else if constexpr (std::is_same_v<T, double  >) fprintf(stream.file, "%f", v);
            else fprintf(stream.file, "? %s\n", typeid(T).name());
        } else {
            // text mode read not implemented
            assert("nope");
        }
    }

    if (stream.mode == PACK_MODE_WRITE) {
        fflush(stream.file);
    }
}
void Process(PackStream &stream, std::string &str)
{
    uint16_t strLen = (uint16_t)str.size();
    if (stream.type == PACK_TYPE_TEXT) {
        if (stream.mode == PACK_MODE_WRITE) {
            fprintf(stream.file, "str %s\n", str.c_str());
        } else {
            // text mode read not implemented
            assert("nope");
        }
    } else {
        PROC(strLen);
        str.resize(strLen);
        for (int i = 0; i < strLen; i++) {
            PROC(str[i]);
        }
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
        //ReadFileIntoDataBuffer(gfx_file.path.c_str(), gfx_file.data_buffer);
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
        //ReadFileIntoDataBuffer(mus_file.path.c_str(), mus_file.data_buffer);
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
void Process(PackStream &stream, Sprite &sprite, int index) {
    PROC(sprite.id);
    PROC(sprite.name);
    assert(ARRAY_SIZE(sprite.anims) == 8); // if this changes, version must increment
    for (int i = 0; i < 8; i++) {
        PROC(sprite.anims[i]);
    }

    if (stream.mode == PACK_MODE_READ) {
        stream.pack->sprite_by_id[sprite.id] = index;
        stream.pack->sprite_by_name[sprite.name] = index;
    }
}
void Process(PackStream &stream, TileMat &tile_mat, int index)
{
    PROC(tile_mat.id);
    PROC(tile_mat.footstep_sound);

    if (stream.mode == PACK_MODE_READ) {
        stream.pack->tile_mat_by_id[tile_mat.id] = index;
        stream.pack->tile_mat_by_name[tile_mat.name] = index;
    }
}
void Process(PackStream &stream, TileDef &tile_def, int index) {
    PROC(tile_def.id);
    PROC(tile_def.name);
    PROC(tile_def.anim);
    PROC(tile_def.material_id);
    PROC(tile_def.flags);
    PROC(tile_def.auto_tile_mask);

    // TODO: Idk where/how to do this, but we don't have the texture
    //       loaded yet in this context, necessarily.
    //tileDef.color = GetImageColor(texEntry.image, tileDef.x, tileDef.y);

    if (stream.mode == PACK_MODE_READ) {
        stream.pack->tile_def_by_id[tile_def.id] = index;
        stream.pack->tile_def_by_name[tile_def.name] = index;
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

    PROC(entity.sprite_id);
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
    PROC(tile_map.name);
    PROC(tile_map.width);
    PROC(tile_map.height);
    PROC(tile_map.title);
    PROC(tile_map.background_music);

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    //SetTextureWrap(texEntry.texture, TEXTURE_WRAP_CLAMP);

    //if (!width || !height) {
    //    err = RN_INVALID_SIZE; break;
    //}

    uint32_t objDataCount   = tile_map.object_data.size();
    uint32_t pathNodeCount  = tile_map.pathNodes.size();
    uint32_t pathCount      = tile_map.paths.size();

    PROC(objDataCount);
    PROC(pathNodeCount);
    PROC(pathCount);

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    tile_map.tiles      .resize((size_t)tile_map.width * tile_map.height);
    tile_map.objects    .resize((size_t)tile_map.width * tile_map.height);
    tile_map.object_data.resize(objDataCount);
    tile_map.pathNodes  .resize(pathNodeCount);
    tile_map.paths      .resize(pathCount);

    for (uint32_t &tile_id : tile_map.tiles) {
        PROC(tile_id);
    }

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    for (uint32_t &obj_id : tile_map.objects) {
        PROC(obj_id);
    }

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    for (ObjectData &obj_data : tile_map.object_data) {
        PROC(obj_data.x);
        PROC(obj_data.y);
        PROC(obj_data.type);

        if (obj_data.type == "lever") {
            PROC(obj_data.power_level);
            PROC(obj_data.tile_def_unpowered);
            PROC(obj_data.tile_def_powered);
        } else if (obj_data.type == "lootable") {
            PROC(obj_data.loot_table_id);
        } else if (obj_data.type == "sign") {
            PROC(obj_data.sign_text[0]);
            PROC(obj_data.sign_text[1]);
            PROC(obj_data.sign_text[2]);
            PROC(obj_data.sign_text[3]);
        } else if (obj_data.type == "warp") {
            PROC(obj_data.warp_map_id);
            PROC(obj_data.warp_dest_x);
            PROC(obj_data.warp_dest_y);
            PROC(obj_data.warp_dest_z);
        }
    }

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    for (AiPathNode &aiPathNode : tile_map.pathNodes) {
        PROC(aiPathNode.pos.x);
        PROC(aiPathNode.pos.y);
        if (tile_map.version >= 9) {
            PROC(aiPathNode.pos.z);
        }
        PROC(aiPathNode.waitFor);
    }

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    for (AiPath &aiPath : tile_map.paths) {
        PROC(aiPath.pathNodeStart);
        PROC(aiPath.pathNodeCount);
    }

    PROC(sentinel);
    assert(sentinel == Tilemap::SENTINEL);

    if (stream.mode == PACK_MODE_READ) {
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

    int tocOffsetPos = ftell(stream.file);
    pack.toc_offset = 0;
    PROC(pack.toc_offset);

    if (stream.mode == PACK_MODE_WRITE) {
        #define WRITE_ARRAY(arr) \
            for (int i = 0; i < arr.size(); i++) { \
                auto &entry = arr[i]; \
                pack.toc.entries.push_back(PackTocEntry(entry.dtype, ftell(stream.file))); \
                Process(stream, entry, i); \
            }

        WRITE_ARRAY(pack.gfx_files);
        WRITE_ARRAY(pack.mus_files);
        WRITE_ARRAY(pack.sfx_files);
        WRITE_ARRAY(pack.gfx_frames);
        WRITE_ARRAY(pack.gfx_anims);
        WRITE_ARRAY(pack.tile_mats);
        WRITE_ARRAY(pack.sprites);
        WRITE_ARRAY(pack.tile_defs);
        WRITE_ARRAY(pack.tile_maps);
        WRITE_ARRAY(pack.entities);

        #undef WRITE_ARRAY

        if (stream.type == PACK_TYPE_BINARY) {
            int tocOffset = ftell(stream.file);
            int entryCount = (int)pack.toc.entries.size();
            PROC(entryCount);
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC((uint8_t &)tocEntry.dtype);
                PROC(tocEntry.offset);
            }

            fseek(stream.file, tocOffsetPos, SEEK_SET);
            PROC(tocOffset);
        }
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

        pack.gfx_files .resize((size_t)typeCounts[DAT_TYP_GFX_FILE]);
        pack.mus_files .resize((size_t)typeCounts[DAT_TYP_MUS_FILE]);
        pack.sfx_files .resize((size_t)typeCounts[DAT_TYP_SFX_FILE]);
        pack.gfx_frames.resize((size_t)typeCounts[DAT_TYP_GFX_FRAME]);
        pack.gfx_anims .resize((size_t)typeCounts[DAT_TYP_GFX_ANIM]);
        pack.tile_mats .resize((size_t)typeCounts[DAT_TYP_TILE_MAT]);
        pack.sprites   .resize((size_t)typeCounts[DAT_TYP_SPRITE]);
        pack.tile_defs .resize((size_t)typeCounts[DAT_TYP_TILE_DEF]);
        pack.tile_maps .resize((size_t)typeCounts[DAT_TYP_TILE_MAP]);
        pack.entities  .resize((size_t)typeCounts[DAT_TYP_ENTITY]);

        int typeNextIndex[DAT_TYP_COUNT]{};

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
                case DAT_TYP_TILE_MAT:  Process(stream, pack.tile_mats [index], index); break;
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
    PerfTimer t{ TextFormat("Save pack %s", pack.path.c_str()) };
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
    {
        PerfTimer t{ "Process" };
        err = Process(stream);
    }

    fclose(stream.file);

    if (err) {
        assert(!err);
        TraceLog(LOG_ERROR, "Failed to save pack file.\n");
    }
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

    {
        PerfTimer t{ "Load graphics" };
        for (GfxFile &gfx_file : pack.gfx_files) {
            if (gfx_file.path.empty()) continue;
            gfx_file.texture = LoadTexture(gfx_file.path.c_str());
            SetTextureFilter(gfx_file.texture, TEXTURE_FILTER_POINT);
        }
    }
    {
        PerfTimer t{ "Load music" };
        for (MusFile &mus_file : pack.mus_files) {
            if (mus_file.path.empty()) continue;
            mus_file.music = LoadMusicStream(mus_file.path.c_str());
        }
    }
    {
        PerfTimer t{ "Load sounds" };
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
    }

    return err;
}
Err LoadPack(Pack &pack, PackStreamType type)
{
    PerfTimer t{ TextFormat("Load pack %s", pack.path.c_str()) };
    Err err = RN_SUCCESS;

    FILE *file = fopen(pack.path.c_str(), type == PACK_TYPE_BINARY ? "rb" : "r");
    if (!file) {
        return RN_BAD_FILE_READ;
    }

    PackStream stream{ &pack, file, PACK_MODE_READ, type };
    do {
        {
            PerfTimer t{ "Process" };
            err = Process(stream);
        }
        if (err) break;

        {
            PerfTimer t{ "LoadResources" };
            err = LoadResources(*stream.pack);
        }
        if (err) break;
    } while(0);

    if (stream.file) {
        fclose(stream.file);
    }

    if (err) {
        TraceLog(LOG_ERROR, "Failed to load data file.\n");
    }
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
    const SfxFile &sfx_file = packs[0].FindSoundVariant(id);
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
    const SfxFile &sfx_file = packs[0].FindSoundVariant(id);
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
    const SfxFile &sfx_file = packs[0].FindSoundVariant(id);
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

void UpdateSprite(Entity &entity, double dt, bool newlySpawned)
{
    // TODO: Make this more general and stop taking in entityType.
    switch (entity.type) {
        case ENTITY_PLAYER: case ENTITY_NPC: {
            if (entity.velocity.x > 0) {
                entity.direction = DIR_E;
            } else if (entity.velocity.x < 0) {
                entity.direction = DIR_W;
            } else if (!entity.direction) {
                entity.direction = DIR_E;  // HACK: we don't have north sprites atm
            }
            break;
        }
    }

    const Sprite &sprite = packs[0].FindSprite(entity.sprite_id);
    const GfxAnim &anim = packs[0].FindGraphicAnim(sprite.anims[entity.direction]);
    UpdateGfxAnim(anim, dt, entity.anim_state);

    if (newlySpawned && anim.sound != "null") {
        const SfxFile &sfx_file = packs[0].FindSoundVariant(anim.sound);
        PlaySound(sfx_file.id);
    }
}
void ResetSprite(Entity &entity)
{
    const Sprite &sprite = packs[0].FindSprite(entity.sprite_id);
    GfxAnim &anim = packs[0].FindGraphicAnim(sprite.anims[entity.direction]);
    StopSound(anim.sound);

    entity.anim_state.frame = 0;
    entity.anim_state.accum = 0;
}
void DrawSprite(const Entity &entity, DrawCmdQueue *sortedDraws, bool highlight)
{
    const GfxFrame &frame = entity.GetSpriteFrame();
    const GfxFile &gfx_file = packs[0].FindGraphic(frame.gfx);

    Rectangle sprite_rect = entity.GetSpriteRect();
    Vector3 pos = { sprite_rect.x, sprite_rect.y };
    pos.x = floorf(pos.x);
    pos.y = floorf(pos.y);
    //if (Vector3LengthSqr(entity.velocity) < 0.0001f) {
    //    pos.x = floorf(pos.x);
    //    pos.y = floorf(pos.y);
    //}

    Color color = WHITE;
    if (entity.on_warp_cooldown) {
        color = ColorTint(color, SKYBLUE);
    }
    if (highlight) {
        color = ColorBrightness(color, 1.3f);
    }

    const Rectangle frame_rec{ (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h };
    if (sortedDraws) {
        DrawCmd cmd{};
        cmd.texture = gfx_file.texture;
        cmd.rect = frame_rec;
        cmd.position = pos;
        cmd.color = color;
        sortedDraws->push(cmd);
    } else {
        const Vector2 sprite_pos{ pos.x, pos.y - pos.z };
        dlb_DrawTextureRec(gfx_file.texture, frame_rec, sprite_pos, color);
    }
}

void UpdateTileDefAnimations(double dt)
{
    for (TileDef &tile_def : packs[0].tile_defs) {
        const GfxAnim &anim = packs[0].FindGraphicAnim(tile_def.anim);
        UpdateGfxAnim(anim, dt, tile_def.anim_state);
    }
}

ENUM_STR_CONVERTER(DataType,      DATA_TYPES,     ENUM_VD_CASE_RETURN_DESC);
ENUM_STR_CONVERTER(EntityType,    ENTITY_TYPES,   ENUM_V_CASE_RETURN_VSTR);
ENUM_STR_CONVERTER(EntitySpecies, ENTITY_SPECIES, ENUM_V_CASE_RETURN_VSTR);
ENUM_STR_CONVERTER(SchemaType,    SCHEMA_TYPES,   ENUM_V_CASE_RETURN_VSTR);
#undef ENUM_STR_GENERATOR
