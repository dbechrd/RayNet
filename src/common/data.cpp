#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "perf_timer.h"
#include "schema.h"

#define MD_HIJACK_STRING_CONSTANTS 0
#include "md.h"

//#include "capnp/message.h"
//#include "capnp/serialize.h"
//#include "capnp/serialize-text.h"
//#include "meta/resource_library.capnp.h"
//
//#include <io.h>
//#include <fcntl.h>

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

struct MetaParserBase {
protected:
    MD_Arena *arena;
    MD_ParseResult parse_result;

    MetaParserBase(void) {
        arena = MD_ArenaAlloc();
    }

    ~MetaParserBase(void) {
        MD_ArenaRelease(arena);
    }

    void ParseWholeFile(const char *filename)
    {
        MD_String8 md_filename = MD_S8CString((char *)filename);
        parse_result = MD_ParseWholeFile(arena, md_filename);
    }

    void PrintNode(FILE *file, MD_Node *node, MD_GenerateFlags flags)
    {
        MD_ArenaTemp scratch = MD_GetScratch(0, 0);
        MD_String8List list = {0};
        MD_DebugDumpFromNode(scratch.arena, &list, node, 0, MD_S8Lit("    "), flags);
        MD_String8 string = MD_S8ListJoin(scratch.arena, list, 0);
        fwrite(string.str, string.size, 1, file);
        MD_ReleaseScratch(scratch);
    }
    void Print(void)
    {
        PrintNode(stdout, parse_result.node, MD_GenerateFlags_All);
    }

    void PrintErrors(MD_ParseResult *parse_result)
    {
        for (MD_Message *message = parse_result->errors.first; message; message = message->next) {
            MD_CodeLoc code_loc = MD_CodeLocFromNode(message->node);
            MD_PrintMessage(stderr, code_loc, message->kind, message->string);
        }
    }
    bool Expect_Ident(MD_Node *node, std::string *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Identifier) {
            if (result) {
                *result = std::string((char *)node->string.str, node->string.size);
            }
            return true;
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected identifier."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }
    }
    bool Expect_String(MD_Node *node, std::string *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_StringLiteral) {
            if (result) {
                *result = std::string((char *)node->string.str, node->string.size);
            }
            return true;
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected double-quoted string."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }
    }
    bool Expect_Int(MD_Node *node, int min, int max, int *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric && MD_StringIsCStyleInt(node->string)) {
            int val = (int)MD_CStyleIntFromString(node->string);
            if (val >= min && val <= max) {
                if (result) {
                    *result = val;
                }
                return true;
            } else {
                MD_String8 err = MD_S8Fmt(arena, (char *)"Expected integer in inclusive range [%u, %u].", min, max);
                MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, err);
                MD_MessageListPush(&parse_result.errors, md_err);
                return false;
            }
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected integer literal."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }

    }
    bool Expect_Uint_internal(MD_Node *node, uint32_t min, uint32_t max, uint32_t *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric && MD_StringIsCStyleInt(node->string)) {
            MD_u64 val = MD_U64FromString(node->string, 10);
            if (val >= min && val <= max) {
                if (result) {
                    *result = val;
                }
                return true;
            } else {
                MD_String8 err = MD_S8Fmt(arena, (char *)"Expected unsigned integer in inclusive range [%u, %u].", min, max);
                MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, err);
                MD_MessageListPush(&parse_result.errors, md_err);
                return false;
            }
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected unsigned integer literal."));
            return false;
        }
    }
    bool Expect_Uint8(MD_Node *node, uint8_t *result)
    {
        uint32_t val = 0;
        if (Expect_Uint_internal(node, 0, UINT8_MAX, &val)) {
            *result = (uint8_t)val;
            return true;
        }
        return false;
    }
    bool Expect_Uint16(MD_Node *node, uint16_t *result)
    {
        uint32_t val = 0;
        if (Expect_Uint_internal(node, 0, UINT16_MAX, &val)) {
            *result = (uint16_t)val;
            return true;
        }
        return false;
    }
    bool Expect_Uint32(MD_Node *node, uint32_t *result)
    {
        uint32_t val = 0;
        if (Expect_Uint_internal(node, 0, UINT32_MAX, &val)) {
            *result = (uint32_t)val;
            return true;
        }
        return false;
    }
    bool Expect_Float(MD_Node *node, float *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric) {
            if (result) {
                *result = (float)MD_F64FromString(node->string);
            }
            return true;
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected float literal."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }
    }
    bool Expect_Double(MD_Node *node, double *result)
    {
        if (node->kind == MD_NodeKind_Main && node->flags & MD_NodeFlag_Numeric) {
            if (result) {
                *result = MD_F64FromString(node->string);
            }
            return true;
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected double literal."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }
    }
    bool Expect_Nil(MD_Node *node)
    {
        if (MD_NodeIsNil(node)) {
            return true;
        } else {
            MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8Lit("Expected end of scope."));
            MD_MessageListPush(&parse_result.errors, md_err);
            return false;
        }
    }
};

#define META_ID_STR(dest) \
if (!Expect_Ident(node, &dest)) break;
#define META_ID_UINT32(dest) \
if (!Expect_Uint32(node, &dest)) break;
#define META_IDENT(dest) \
    if (!Expect_Ident(node, &dest)) break; \
    node = node->next;
#define META_STRING(dest) \
    if (!Expect_String(node, &dest)) break; \
    node = node->next;
#define META_UINT8(dest) \
    if (!Expect_Uint8(node, &dest)) break; \
    node = node->next;
#define META_UINT16(dest) \
    if (!Expect_Uint16(node, &dest)) break; \
    node = node->next;
#define META_UINT32(dest) \
    if (!Expect_Uint32(node, &dest)) break; \
    node = node->next;
#define META_INT(dest) \
    if (!Expect_Int(node, INT32_MIN, INT32_MAX, &dest)) break; \
    node = node->next;
#define META_FLOAT(dest) \
    if (!Expect_Float(node, &dest)) break; \
    node = node->next;
#define META_DOUBLE(dest) \
    if (!Expect_Double(node, &dest)) break; \
    node = node->next;
#define META_CHILDREN_BEGIN \
do { \
    MD_Node *parent = node; \
    MD_Node *node = parent->first_child;
#define META_CHILDREN_END \
    Expect_Nil(node); \
} while(0);
#define META_CHILDREN_LOOP_BEGIN \
while (!MD_NodeIsNil(node)) {

#define META_CHILDREN_LOOP_END \
}
#define META_CHILDREN_LOOP_END_NEXT \
    node = node->next; \
}

struct MetaParser : public MetaParserBase {
    MetaParser(void) = default;

    static void ParseIntoPack(Pack &pack, const char *filename) {
        MetaParser parser{};
        parser.InternalParseIntoPack(pack, filename);
    }

    void InternalParseIntoPack(Pack &pack, const char *filename)
    {
        ParseWholeFile(filename);

        if (!parse_result.errors.node_count) {
            //Print();
            for (MD_EachNode(node, parse_result.node->first_child)) {
                auto tag_count = MD_TagCountFromNode(node);
                if (tag_count != 1) {
                    MD_Message *md_err = MD_MakeNodeError(arena, node, MD_MessageKind_Error, MD_S8CString((char *)"Expected exactly 1 tag."));
                    MD_MessageListPush(&parse_result.errors, md_err);
                    break;
                }

                if (MD_NodeHasTag(node, MD_S8Lit("GfxFile"), 0)) {
                    GfxFile gfx_file{};

                    META_ID_UINT32(gfx_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(gfx_file.name);
                        META_STRING(gfx_file.path);
                    META_CHILDREN_END;

                    pack.gfx_files.push_back(gfx_file);
                } else if (MD_NodeHasTag(node, MD_S8Lit("MusFile"), 0)) {
                    MusFile mus_file{};

                    META_ID_UINT32(mus_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(mus_file.name);
                        META_STRING(mus_file.path);
                    META_CHILDREN_END;

                    pack.mus_files.push_back(mus_file);
                } else if (MD_NodeHasTag(node, MD_S8Lit("SfxFile"), 0)) {
                    SfxFile sfx_file{};

                    META_ID_UINT32(sfx_file.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(sfx_file.name);
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

                    META_ID_UINT32(gfx_frame.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(gfx_frame.name);
                        META_IDENT(gfx_frame.gfx);
                        META_UINT16(gfx_frame.x);
                        META_UINT16(gfx_frame.y);
                        META_UINT16(gfx_frame.w);
                        META_UINT16(gfx_frame.h);
                    META_CHILDREN_END;

                    pack.gfx_frames.push_back(gfx_frame);
                } else if (MD_NodeHasTag(node, MD_S8Lit("GfxAnim"), 0)) {
                    GfxAnim gfx_anim{};

                    META_ID_UINT32(gfx_anim.id);
                    META_CHILDREN_BEGIN;
                        META_STRING(gfx_anim.name);
                        META_IDENT(gfx_anim.sound);
                        META_UINT8(gfx_anim.frame_delay);

                        int frame_count = 0;
                        META_INT(frame_count);
                        std::string frame_name;
                        for (int i = 0; i < frame_count; i++) {
                            META_IDENT(frame_name);
                            gfx_anim.frames.push_back(frame_name);
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

        if (parse_result.errors.node_count) {
            PrintErrors(&parse_result);
            assert(!"MD parse failed");
        }
    }
};

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
#if HAQ_ENABLE_SCHEMA
    haq_print(hqt_gfx_file_schema);
    printf("");
#endif

#if 0
    // TODO: Investigate POD plugin
    // https://github.com/nickolasrossi/podgen/tree/master
    {
        capnp::MallocMessageBuilder message;

        meta::ResourceLibrary::Builder library = message.initRoot<meta::ResourceLibrary>();
        capnp::List<meta::Resource>::Builder resources = library.initResources(2);

        meta::Resource::Builder gfx_missing = resources[0];
        gfx_missing.setId(0);
        gfx_missing.setName("gfx_missing");
        gfx_missing.setPath("resources/graphics/missing.png");

        meta::Resource::Builder gfx_dlg_npatch = resources[1];
        gfx_dlg_npatch.setId(1);
        gfx_dlg_npatch.setName("gfx_dlg_npatch");
        gfx_dlg_npatch.setPath("resources/graphics/npatch.png");

        int fd = _open("capnp/resource_library.bin", _O_RDWR | _O_CREAT | _O_BINARY);
        capnp::writeMessageToFd((int)fd, message);
        _close(fd);
    }

    {
        int fd = _open("capnp/resource_library.bin", _O_RDWR | _O_BINARY);
        capnp::StreamFdMessageReader reader(fd);

        auto library = reader.getRoot<meta::ResourceLibrary>();
        for (auto resource : library.getResources()) {
            printf("resource name: %s\n", resource.getName().cStr());
        }

        _close(fd);
    }
#endif

#if 0
    NewEnt::run_tests();
#endif

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

#if 1
    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    if (!FileExists(fontName)) {
        fontName = "resources/font/KarminaBold.otf";
    }
#else
    //const char *fontName = "resources/font/KarminaBold.otf";
    //const char *fontName = "resources/font/PixelOperator-Bold.ttf";
    //const char *fontName = "resources/font/PixelOperatorMono-Bold.ttf";
    const char *fontName = "resources/font/FiraMono-Medium.ttf";
#endif

    fntTiny = dlb_LoadFontEx(fontName, 12, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntTiny.baseSize, RN_RAYLIB_ERROR);

    fntSmall = dlb_LoadFontEx(fontName, 15, 0, 0, FONT_DEFAULT);
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

            MetaParser::ParseIntoPack(packAssets, "resources/meta/overworld.mdesk");
            MetaParser::ParseIntoPack(packMaps, "resources/map/map_overworld.mdesk");
            MetaParser::ParseIntoPack(packMaps, "resources/map/map_cave.mdesk");
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

        Pack packAssetsTxt{ "pack/assets.txt" };
        ERR_RETURN(LoadPack(packAssetsTxt, PACK_TYPE_TEXT));
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

#define PROC(v) Process(stream, v)

template<class T>
void Process(PackStream &stream, T &v)
{
    static_assert(std::is_fundamental_v<T> || std::is_enum_v<T>,
        "You cannot fread/write a non-primitive type");

    if (stream.type == PACK_TYPE_BINARY) {
        stream.process(&v, sizeof(v), 1, stream.file);
    } else if (stream.type == PACK_TYPE_TEXT) {
        if (stream.mode == PACK_MODE_WRITE) {
            if      constexpr (std::is_same_v<T, char    >) fprintf(stream.file, " %c", v);
            else if constexpr (std::is_same_v<T, int8_t  >) fprintf(stream.file, " %" PRId8 , v);
            else if constexpr (std::is_same_v<T, int16_t >) fprintf(stream.file, " %" PRId16, v);
            else if constexpr (std::is_same_v<T, int32_t >) fprintf(stream.file, " %" PRId32, v);
            else if constexpr (std::is_same_v<T, int64_t >) fprintf(stream.file, " %" PRId64, v);
            else if constexpr (std::is_same_v<T, uint8_t >) fprintf(stream.file, " %" PRIu8 , v);
            else if constexpr (std::is_same_v<T, uint16_t>) fprintf(stream.file, " %" PRIu16, v);
            else if constexpr (std::is_same_v<T, uint32_t>) fprintf(stream.file, " %" PRIu32, v);
            else if constexpr (std::is_same_v<T, uint64_t>) fprintf(stream.file, " %" PRIu64, v);
            else if constexpr (std::is_same_v<T, float   >) fprintf(stream.file, " %f", v);
            else if constexpr (std::is_same_v<T, double  >) fprintf(stream.file, " %f", v);
            else if constexpr (std::is_enum_v<T>)           fprintf(stream.file, " %d", v);
            else assert(!"unhandled type"); //fprintf(stream.file, " <%s>\n", typeid(T).name());
        } else {
            if      constexpr (std::is_same_v<T, char    >) fscanf(stream.file, " %c", &v);
            else if constexpr (std::is_same_v<T, int8_t  >) fscanf(stream.file, " %" PRId8 , &v);
            else if constexpr (std::is_same_v<T, int16_t >) fscanf(stream.file, " %" PRId16, &v);
            else if constexpr (std::is_same_v<T, int32_t >) fscanf(stream.file, " %" PRId32, &v);
            else if constexpr (std::is_same_v<T, int64_t >) fscanf(stream.file, " %" PRId64, &v);
            else if constexpr (std::is_same_v<T, uint8_t >) fscanf(stream.file, " %" PRIu8 , &v);
            else if constexpr (std::is_same_v<T, uint16_t>) fscanf(stream.file, " %" PRIu16, &v);
            else if constexpr (std::is_same_v<T, uint32_t>) fscanf(stream.file, " %" PRIu32, &v);
            else if constexpr (std::is_same_v<T, uint64_t>) fscanf(stream.file, " %" PRIu64, &v);
            else if constexpr (std::is_same_v<T, float   >) fscanf(stream.file, " %f", &v);
            else if constexpr (std::is_same_v<T, double  >) fscanf(stream.file, " %lf", &v);
            else if constexpr (std::is_enum_v<T>) {
                int val = -1;
                fscanf(stream.file, " %d", &val);
                v = (T)val;  // note(dlb): might be out of range, but idk how to detect that. C++ sucks.
            }
            else assert(!"unhandled type"); //fprintf(stream.file, " <%s>\n", typeid(T).name());
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
            fprintf(stream.file, " \"%s\"", str.c_str());
        } else {
            char buf[1024]{};
            fscanf(stream.file, " \"%1023[^\"]", buf);
            char end_quote{};
            fscanf(stream.file, "%c", &end_quote);
            if (end_quote != '"') {
                assert(!"string too long, not supported");
            }
            str = buf;
        }
    } else {
        PROC(strLen);
        str.resize(strLen);
        for (int i = 0; i < strLen; i++) {
            PROC(str[i]);
        }
    }
}
template <typename T, size_t S>
void Process(PackStream &stream, std::array<T, S> &arr)
{
    assert(arr.size() < UINT16_MAX);
    uint16_t len = (uint16_t)arr.size();
    PROC(len);
    assert(len == S);
    for (int i = 0; i < len; i++) {
        PROC(arr[i]);
    }
}
template <typename T>
void Process(PackStream &stream, std::vector<T> &vec)
{
    assert(vec.size() < UINT16_MAX);
    uint16_t len = (uint16_t)vec.size();
    PROC(len);
    vec.resize(len);
    for (int i = 0; i < len; i++) {
        PROC(vec[i]);
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

#define HAQ_IO_TYPE(c_type, c_type_name, c_body, userdata) \
    c_body

#define HAQ_IO_FIELD(c_type, c_name, c_init, flags, userdata) \
    if constexpr ((flags) & HAQ_SERIALIZE) { \
        PROC(userdata.c_name); \
    }

#define HAQ_IO(hqt, userdata) \
    hqt(HAQ_IO_TYPE, HAQ_IO_FIELD, HAQ_IGNORE, userdata);

void Process(PackStream &stream, GfxFile &gfx_file)
{
    HAQ_IO(HQT_GFX_FILE, gfx_file);
}
void Process(PackStream &stream, MusFile &mus_file)
{
    HAQ_IO(HQT_MUS_FILE, mus_file);
}
void Process(PackStream &stream, SfxFile &sfx_file)
{
    HAQ_IO(HQT_SFX_FILE, sfx_file);
}
void Process(PackStream &stream, GfxFrame &gfx_frame)
{
    HAQ_IO(HQT_GFX_FRAME, gfx_frame);
}
void Process(PackStream &stream, GfxAnim &gfx_anim)
{
    HAQ_IO(HQT_GFX_ANIM, gfx_anim);
}
void Process(PackStream &stream, Sprite &sprite)
{
    HAQ_IO(HQT_SPRITE, sprite);
}
void Process(PackStream &stream, TileDef &tile_def)
{
    HAQ_IO(HQT_TILE_DEF, tile_def);
}
void Process(PackStream &stream, TileMat &tile_mat)
{
    HAQ_IO(HQT_TILE_MAT, tile_mat);
}
void Process(PackStream &stream, Tilemap &tile_map)
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
}
void Process(PackStream &stream, Entity &entity)
{
    bool alive = entity.id && !entity.despawned_at && entity.type;
    PROC(alive);
    if (!alive) {
        return;
    }

    //// Entity ////
    PROC(entity.id);
    PROC(entity.type);
    PROC(entity.spec);
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
    PROC(entity.direction);
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

template <typename T>
void AddToIndex(Pack &pack, T &dat, size_t index)
{
    const auto &by_id = pack.dat_by_id[T::dtype];
    pack.dat_by_id[T::dtype][dat.id] = index;

    const auto &by_name = pack.dat_by_name[T::dtype];
    pack.dat_by_name[T::dtype][dat.name] = index;
}

template <typename T>
void WriteArrayBin(PackStream &stream, std::vector<T> &vec)
{
    for (T &entry : vec) {
        stream.pack->toc.entries.push_back(PackTocEntry(entry.dtype, ftell(stream.file)));
        PROC(entry);
    }
}

template <typename T>
void ReadEntryBin(PackStream &stream, size_t index)
{
    auto &vec = *(std::vector<T> *)stream.pack->GetPool(T::dtype);
    auto &dat = vec[index];
    PROC(dat);

    if (stream.mode == PACK_MODE_READ) {
        AddToIndex(*stream.pack, dat, index);
    }
}

template <typename T>
void WriteArrayTxt(PackStream &stream, std::vector<T> &vec)
{
    char newline = '\n';
    for (T &entry : vec) {
        DataType dtype = entry.dtype;
        PROC(dtype);
        PROC(entry);
        PROC(newline);
    }
}

void IgnoreCommentsTxt(FILE *f)
{
    char next = fgetc(f);
    while (isspace(next)) {
        next = fgetc(f);
    }
    while (next == '#') {
        while (next != '\n') {
            next = fgetc(f);
        }
        next = fgetc(f);
    }
    fseek(f, ftell(f) - 1, SEEK_SET);
}

template <typename T>
void ReadEntryTxt(PackStream &stream)
{
    auto &vec = *(std::vector<T> *)stream.pack->GetPool(T::dtype);
    size_t index = vec.size();
    auto &dat = vec.emplace_back();
    PROC(dat);

    if (stream.mode == PACK_MODE_READ) {
        AddToIndex(*stream.pack, dat, index);
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
    if (stream.type == PACK_TYPE_BINARY) {
        PROC(pack.toc_offset);
    }

    if (stream.mode == PACK_MODE_WRITE) {
        if (stream.type == PACK_TYPE_BINARY) {
            WriteArrayBin(stream, pack.gfx_files);
            WriteArrayBin(stream, pack.mus_files);
            WriteArrayBin(stream, pack.sfx_files);
            WriteArrayBin(stream, pack.gfx_frames);
            WriteArrayBin(stream, pack.gfx_anims);
            WriteArrayBin(stream, pack.sprites);
            WriteArrayBin(stream, pack.tile_defs);
            WriteArrayBin(stream, pack.tile_mats);
            WriteArrayBin(stream, pack.tile_maps);
            WriteArrayBin(stream, pack.entities);

            int tocOffset = ftell(stream.file);
            uint32_t entryCount = (uint32_t)pack.toc.entries.size();
            PROC(entryCount);
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
            }

            fseek(stream.file, tocOffsetPos, SEEK_SET);
            PROC(tocOffset);
        } else {
            fprintf(stream.file, "\n");

            #define HAQ_TXT_DOC_COMMENT_TYPE(c_type, c_type_name, c_body, userdata) \
                fprintf(stream.file, "\n# %s ", #c_type_name); \
                c_body

            #define HAQ_TXT_DOC_COMMENT_FIELD(c_type, c_name, c_init, flags, userdata) \
                if ((flags) & HAQ_SERIALIZE) { \
                    fprintf(stream.file, "%s ", #c_name); \
                }

            #define HAQ_TXT_DOC_COMMENT(hqt) \
                hqt(HAQ_TXT_DOC_COMMENT_TYPE, HAQ_TXT_DOC_COMMENT_FIELD, HAQ_IGNORE, 0); \
                fprintf(stream.file, "\n");

            HAQ_TXT_DOC_COMMENT(HQT_GFX_FILE);
            WriteArrayTxt(stream, pack.gfx_files);
            HAQ_TXT_DOC_COMMENT(HQT_MUS_FILE);
            WriteArrayTxt(stream, pack.mus_files);
            HAQ_TXT_DOC_COMMENT(HQT_SFX_FILE);
            WriteArrayTxt(stream, pack.sfx_files);
            HAQ_TXT_DOC_COMMENT(HQT_GFX_FRAME);
            WriteArrayTxt(stream, pack.gfx_frames);
            HAQ_TXT_DOC_COMMENT(HQT_GFX_ANIM);
            WriteArrayTxt(stream, pack.gfx_anims);
            WriteArrayTxt(stream, pack.sprites);
            WriteArrayTxt(stream, pack.tile_defs);
            HAQ_TXT_DOC_COMMENT(HQT_TILE_MAT);
            WriteArrayTxt(stream, pack.tile_mats);
            WriteArrayTxt(stream, pack.tile_maps);
            WriteArrayTxt(stream, pack.entities);

            #undef HAQ_TXT_DOC_COMMENT_TYPE
            #undef HAQ_TXT_DOC_COMMENT_FIELD
            #undef HAQ_TXT_DOC_COMMENT

            fprintf(stream.file, "\n# EOF marker\n%d", -1);
        }
    } else {
        if (stream.type == PACK_TYPE_BINARY) {
            fseek(stream.file, pack.toc_offset, SEEK_SET);
            uint32_t entryCount = 0;
            PROC(entryCount);
            pack.toc.entries.resize(entryCount);

            size_t typeCounts[DAT_TYP_COUNT]{};
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
            pack.sprites   .resize(typeCounts[DAT_TYP_SPRITE]);
            pack.tile_defs .resize(typeCounts[DAT_TYP_TILE_DEF]);
            pack.tile_mats .resize(typeCounts[DAT_TYP_TILE_MAT]);
            pack.tile_maps .resize(typeCounts[DAT_TYP_TILE_MAP]);
            pack.entities  .resize(typeCounts[DAT_TYP_ENTITY]);

            size_t typeNextIndex[DAT_TYP_COUNT]{};

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.file, tocEntry.offset, SEEK_SET);
                size_t index = typeNextIndex[tocEntry.dtype];
                tocEntry.index = index;
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  ReadEntryBin<GfxFile> (stream, index); break;
                    case DAT_TYP_MUS_FILE:  ReadEntryBin<MusFile> (stream, index); break;
                    case DAT_TYP_SFX_FILE:  ReadEntryBin<SfxFile> (stream, index); break;
                    case DAT_TYP_GFX_FRAME: ReadEntryBin<GfxFrame>(stream, index); break;
                    case DAT_TYP_GFX_ANIM:  ReadEntryBin<GfxAnim> (stream, index); break;
                    case DAT_TYP_SPRITE:    ReadEntryBin<Sprite>  (stream, index); break;
                    case DAT_TYP_TILE_DEF:  ReadEntryBin<TileDef> (stream, index); break;
                    case DAT_TYP_TILE_MAT:  ReadEntryBin<TileMat> (stream, index); break;
                    case DAT_TYP_TILE_MAP:  ReadEntryBin<Tilemap> (stream, index); break;
                    case DAT_TYP_ENTITY:    ReadEntryBin<Entity>  (stream, index); break;
                }
                typeNextIndex[tocEntry.dtype]++;
            }
        } else {
            while (!feof(stream.file)) {
                int dtype = DAT_TYP_INVALID;
                IgnoreCommentsTxt(stream.file);
                PROC(dtype);
                switch (dtype) {
                    case DAT_TYP_GFX_FILE:  ReadEntryTxt<GfxFile> (stream); break;
                    case DAT_TYP_MUS_FILE:  ReadEntryTxt<MusFile> (stream); break;
                    case DAT_TYP_SFX_FILE:  ReadEntryTxt<SfxFile> (stream); break;
                    case DAT_TYP_GFX_FRAME: ReadEntryTxt<GfxFrame>(stream); break;
                    case DAT_TYP_GFX_ANIM:  ReadEntryTxt<GfxAnim> (stream); break;
                    case DAT_TYP_SPRITE:    ReadEntryTxt<Sprite>  (stream); break;
                    case DAT_TYP_TILE_DEF:  ReadEntryTxt<TileDef> (stream); break;
                    case DAT_TYP_TILE_MAT:  ReadEntryTxt<TileMat> (stream); break;
                    case DAT_TYP_TILE_MAP:  ReadEntryTxt<Tilemap> (stream); break;
                    case DAT_TYP_ENTITY:    ReadEntryTxt<Entity>  (stream); break;
                    case -1: break;
                    default: assert(!"uh-oh");
                }
            }
        }
        // Maybe I care about this one day.. prolly not
        //ReadFileIntoDataBuffer(gfx_file.path.c_str(), gfx_file.data_buffer);
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

Sound PickSoundVariant(SfxFile &sfx_file) {
#if 0
    // TODO: Re-implement this by adding variations_instances or whatever
    size_t sfx_idx;
    if (variants.size() > 1) {
        const size_t variant_idx = (size_t)GetRandomValue(0, variants.size() - 1);
        sfx_idx = variants[variant_idx];
    } else {
        sfx_idx = variants[0];
    }
#else
    return sfx_file.sound;
#endif
}

void PlaySound(const std::string &id, float pitchVariance)
{
    // TODO: FIXME
#if 0
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
#endif
}
bool IsSoundPlaying(const std::string &id)
{
    bool playing = false;
#if 0
    // TODO: Does this work still with SoundAlias stuff?
    const SfxFile &sfx_file = packs[0].FindSoundVariant(id);
    assert(sfx_file.instances.size() == sfx_file.max_instances);

    for (int i = 0; i < sfx_file.max_instances; i++) {
        playing = IsSoundPlaying(sfx_file.instances[i]);
        if (playing) break;
    }
#endif
    return playing;
}
void StopSound(const std::string &id)
{
#if 0
    const SfxFile &sfx_file = packs[0].FindSoundVariant(id);
    assert(sfx_file.instances.size() == sfx_file.max_instances);

    for (int i = 0; i < sfx_file.max_instances; i++) {
        StopSound(sfx_file.instances[i]);
    }
#endif
}

void UpdateGfxAnim(const GfxAnim &anim, double dt, GfxAnimState &anim_state)
{
    anim_state.accum += dt;

    const double anim_frame_dt = 1.0 / 60.0;
    const double frame_time = anim.frame_delay * anim_frame_dt;
    if (anim_state.accum >= frame_time) {
        anim_state.frame++;
        if (anim_state.frame >= anim.frames.size()) {
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

    const Sprite &sprite = packs[0].FindById<Sprite>(entity.sprite_id);
    const GfxAnim &anim = packs[0].FindByName<GfxAnim>(sprite.anims[entity.direction]);
    UpdateGfxAnim(anim, dt, entity.anim_state);

    if (newlySpawned && anim.sound != "null") {
        const SfxFile &sfx_file = packs[0].FindByName<SfxFile>(anim.sound);
        PlaySound(sfx_file.name);  // TODO: Play by id instead
    }
}
void ResetSprite(Entity &entity)
{
    const Sprite &sprite = packs[0].FindById<Sprite>(entity.sprite_id);
    GfxAnim &anim = packs[0].FindByName<GfxAnim>(sprite.anims[entity.direction]);
    StopSound(anim.sound);

    entity.anim_state.frame = 0;
    entity.anim_state.accum = 0;
}
void DrawSprite(const Entity &entity, DrawCmdQueue *sortedDraws, bool highlight)
{
    const GfxFrame &frame = entity.GetSpriteFrame();
    const GfxFile &gfx_file = packs[0].FindByName<GfxFile>(frame.gfx);

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
        const GfxAnim &anim = packs[0].FindByName<GfxAnim>(tile_def.anim);
        UpdateGfxAnim(anim, dt, tile_def.anim_state);
    }
}

ENUM_STR_CONVERTER(DataType,      DATA_TYPES,     ENUM_VD_CASE_RETURN_DESC);
ENUM_STR_CONVERTER(EntityType,    ENTITY_TYPES,   ENUM_V_CASE_RETURN_VSTR);
ENUM_STR_CONVERTER(EntitySpecies, ENTITY_SPECIES, ENUM_V_CASE_RETURN_VSTR);
ENUM_STR_CONVERTER(SchemaType,    SCHEMA_TYPES,   ENUM_V_CASE_RETURN_VSTR);
#undef ENUM_STR_GENERATOR
