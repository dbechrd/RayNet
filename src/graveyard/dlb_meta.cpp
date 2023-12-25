#define MD_HIJACK_STRING_CONSTANTS 0
#include "md.h"

Err SaveTilemap(const std::string &path, Tilemap &tilemap)
{
    Err err = RN_SUCCESS;

    FILE *file = fopen(path.c_str(), "w");
    do {
        if (ferror(file)) {
            err = RN_BAD_FILE_WRITE; break;
        }

        fprintf(file, "@Tilemap %u: {\n", tilemap.id);
        fprintf(file, "    version: %u\n", tilemap.version);
        fprintf(file, "    name: %s\n", tilemap.name.c_str());
        fprintf(file, "    width: %u\n", tilemap.width);
        fprintf(file, "    height: %u\n", tilemap.height);
        fprintf(file, "    title: \"%s\"\n", tilemap.title.c_str());
        fprintf(file, "    bg_music: %s\n", tilemap.bg_music.c_str());
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
        for (AiPathNode &path_node : tilemap.path_nodes) {
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

                    pack.sfx_files.push_back(sfx_file);
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
                    } else if (key == "bg_music") {
                        META_IDENT(map.bg_music);
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

                        map.path_nodes.push_back(path_node);
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