#include "schema.h"

SchemaLibrary g_schema_lib;

void type_field_add(Schema *schema, SchemaType type,
    const char *name, size_t offset, size_t size, size_t array_len,
    bool is_alias, EnumConverter *enum_converter, bool is_union_type,
    bool in_union, int union_type)
{
    SchemaField &field = schema->fields.emplace_back();
    field.type = type;
    field.name = name;
    field.offset = offset;
    field.size = size;
    field.array_len = array_len;
    field.is_alias = is_alias;
    field.enum_converter = enum_converter;
    field.is_union_type = is_union_type;
    field.in_union = in_union;
    field.union_type = union_type;
}

#define TYPE_START(_type, field_type, _init, _free) \
    schema = &g_schema_lib.schemas[field_type]; \
    schema->type = field_type; \
    schema->name = STR(_type); \
    schema->size = sizeof(_type); \
    schema->init = _init; \
    schema->free = _free;

#define TYPE_FIELD(type, field, field_type) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), \
    0, false, 0, false, false, 0)

#define TYPE_FIELD_NAME(type, field, field_type, alias) \
    type_field_add(schema, field_type, #alias, \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), \
    0, false, 0, false, false, 0)

#define TYPE_ENUM(type, field, field_type, converter) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), \
    0, false, converter, false, false, 0)

#define TYPE_ARRAY(type, field, field_type, size) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, field), FIELD_SIZEOF_ARRAY(type, field), \
    size, false, 0, false, false, 0)

#define TYPE_VECTOR(type, field, field_type) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, field), FIELD_SIZEOF_ARRAY(type, field), \
    1, false, 0, false, false, 0)

#define TYPE_UNION_TYPE(type, field, field_type, converter) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, field), FIELD_SIZEOF(type, field), \
    0, false, converter, true, false, 0)

#define TYPE_UNION_FIELD(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF(type, union_name.field), \
    0, false, 0, false, true, union_type)

#define TYPE_UNION_ARRAY(type, field, field_type, size, union_name, union_type) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF_ARRAY(type, union_name.field), \
    size, false, 0, false, true, union_type)

#define TYPE_UNION_VECTOR(type, field, field_type, union_name, union_type) \
    type_field_add(schema, field_type, #field, \
    OFFSETOF(type, union_name.field), FIELD_SIZEOF_ARRAY(type, union_name.field), \
    1, false, 0, false, true, union_type)

#define TYPE_END(type) \
    g_schema_lib.schemas_by_name[STR(type)] = schema; \
    schema = 0;

void ta_schema_register(void)
{
    Schema *schema = 0;

    //--------------------------------------------------------------------------
    // Compound types
    //--------------------------------------------------------------------------
    TYPE_START(Vector2, TYP_VEC2, 0, 0);
    TYPE_FIELD(Vector2, x, TYP_ATOM_FLOAT);
    TYPE_FIELD(Vector2, y, TYP_ATOM_FLOAT);
    TYPE_END  (Vector2);

    TYPE_START(Vector3, TYP_VEC3, 0, 0);
    TYPE_FIELD(Vector3, x, TYP_ATOM_FLOAT);
    TYPE_FIELD(Vector3, y, TYP_ATOM_FLOAT);
    TYPE_FIELD(Vector3, z, TYP_ATOM_FLOAT);
    TYPE_END  (Vector3);

    TYPE_START(Rectangle, TYP_RECT, 0, 0);
    TYPE_FIELD(Rectangle, x,      TYP_ATOM_FLOAT);
    TYPE_FIELD(Rectangle, y,      TYP_ATOM_FLOAT);
    TYPE_FIELD(Rectangle, width,  TYP_ATOM_FLOAT);
    TYPE_FIELD(Rectangle, height, TYP_ATOM_FLOAT);
    TYPE_END  (Rectangle);

    TYPE_START(GfxFile, TYP_GFX_FILE, 0, 0);
    TYPE_FIELD(GfxFile, id,   TYP_ATOM_STRING);
    TYPE_FIELD(GfxFile, path, TYP_ATOM_STRING);
    TYPE_END  (GfxFile);

    TYPE_START(MusFile, TYP_MUS_FILE, 0, 0);
    TYPE_FIELD(MusFile, id,   TYP_ATOM_STRING);
    TYPE_FIELD(MusFile, path, TYP_ATOM_STRING);
    TYPE_END  (MusFile);

    TYPE_START(SfxFile, TYP_SFX_FILE, 0, 0);
    TYPE_FIELD(SfxFile, id,             TYP_ATOM_STRING);
    TYPE_FIELD(SfxFile, path,           TYP_ATOM_STRING);
    TYPE_FIELD(SfxFile, variations,     TYP_ATOM_INT);
    TYPE_FIELD(SfxFile, pitch_variance, TYP_ATOM_FLOAT);
    TYPE_FIELD(SfxFile, max_instances,  TYP_ATOM_INT);
    TYPE_END  (SfxFile);

    TYPE_START(foo, TYP_FOO, 0, 0);
    TYPE_FIELD(foo, dat, TYP_VEC3);
    TYPE_FIELD(foo, sfx, TYP_SFX_FILE);
    TYPE_END  (foo);
}

#undef TYPE_START
#undef TYPE_FIELD
#undef TYPE_FIELD_NAME
#undef TYPE_ENUM
#undef TYPE_ARRAY
#undef TYPE_VECTOR
#undef TYPE_UNION_TYPE
#undef TYPE_UNION_FIELD
#undef TYPE_UNION_ARRAY
#undef TYPE_UNION_VECTOR
#undef TYPE_END

Schema *ta_schema_find_by_name(const std::string &name)
{
    const auto &iter = g_schema_lib.schemas_by_name.find(name);
    if (iter != g_schema_lib.schemas_by_name.end()) {
        return iter->second;
    }
    return 0;
}

SchemaField *ta_schema_field_find(SchemaType type, const std::string &name)
{
    Schema *schema = &g_schema_lib.schemas[type];
    for (SchemaField &field : schema->fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return 0;
}

bool schema_atom_present(SchemaField &field, void *ptr)
{
    bool present = false;
    switch (field.type) {
        case TYP_ATOM_BOOL: {
            bool *val = (bool *)ptr;
            if (*val) present = true;
            break;
        } case TYP_ATOM_UINT8: {
            uint8_t *val = (uint8_t *)ptr;
            if (*val) present = true;
            break;
        } case TYP_ATOM_INT: {
            int32_t *val = (int32_t *)ptr;
            if (*val) present = true;
            break;
        } case TYP_ATOM_UINT: {
            uint32_t *val = (uint32_t *)ptr;
            if (*val) present = true;
            break;
        } case TYP_ATOM_FLOAT: {
            float *val = (float *)ptr;
            if (*val) present = true;
            break;
        } case TYP_ATOM_STRING: {
            std::string *val = (std::string *)ptr;
            if (val->size()) present = true;
            break;
        } case TYP_ATOM_ENUM: {
            //int *val = ptr;
            present = true;
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
    return present;
}

void schema_atom_print(FILE *f, SchemaField &field, void *ptr)
{
    switch (field.type) {
        case TYP_ATOM_BOOL: {
            bool *val = (bool *)ptr;
            fprintf(f, "%s", *val ? "true" : "false");
            break;
        } case TYP_ATOM_UINT8: {
            uint8_t *val = (uint8_t *)ptr;
            fprintf(f, "%u", *val);
            break;
        } case TYP_ATOM_INT: {
            int32_t *val = (int32_t *)ptr;
            fprintf(f, "%d", *val);
            break;
        } case TYP_ATOM_UINT: {
            uint32_t *val = (uint32_t *)ptr;
            fprintf(f, "%u", *val);
            break;
        } case TYP_ATOM_FLOAT: {
            float *val = (float *)ptr;
            fprintf(f, "0x%08X (%f)", *(uint32_t *)val, *val);
            break;
        } case TYP_ATOM_STRING: {
            std::string *val = (std::string *)ptr;
            //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
            fprintf(f, "\"%s\"", val->c_str());
            break;
        } case TYP_ATOM_ENUM: {
            int *val = (int *)ptr;
            fprintf(f, "%d", *val);
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
}

inline void indent(FILE *f, int count)
{
    for (int i = 0; i < count; i++) {
        fprintf(f, "  ");
    }
}

void ta_schema_print(FILE *f, SchemaType type, void *ptr, int level, int in_array)
{
    Schema *schema = &g_schema_lib.schemas[type];
    if (!in_array) {
        if (level == 0) {
            fprintf(f, "%s:\n", schema->name);
        }
    }

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = -9001;

    for (SchemaField &field : schema->fields) {
        if (field.is_alias) {
            continue;
        }
        if (field.in_union && field.union_type != union_type) {
            continue;
        }

        if (field.array_len) {
            //assert(!in_array && "Don't know how to print nested arrays");

            uint8_t *arr = ((uint8_t *)ptr + field.offset);
            size_t arr_len = field.array_len;
            if (arr_len == 1) {
                arr = ((uint8_t *)ptr + field.offset);
                //arr_len = dlb_vec_len(arr);  maybe cast to vector<int> to get the length?
                assert(!"wtf is this? what kind of array?");
            }

            if (arr_len) {
                indent(f, level + 1);
                uint8_t *arr_end = arr + (arr_len * field.size);
                fprintf(f, "%s: [", field.name);
                if (field.type < TYP_COUNT) {
                    fprintf(f, "\n");
                    for (uint8_t *p = arr; p != arr_end; p += field.size) {
                        indent(f, level + 2);
                        fprintf(f, "{\n");
                        ta_schema_print(f, field.type, p, level + 2, in_array + 1);
                        indent(f, level + 2);
                        fprintf(f, "},\n");
                    }
                    indent(f, level + 1);
                } else {
                    for (uint8_t *p = arr; p != arr_end; p += field.size) {
                        schema_atom_print(f, field, p);
                        fprintf(f, ", ");
                    }
                }
                fprintf(f, "]\n");
            } else {
                // Don't print empty arrays?
                //indent(f, level + 1);
                //fprintf(f, "%s: []\n", field.name);
            }
        } else {
            if (field.type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "%s:\n", field.name);
                ta_schema_print(f, field.type, (uint8_t *)ptr + field.offset, level + 1, 0);
                if (in_array) {
                    fprintf(f, ",");
                }
            } else {
                if (field.is_union_type) {
                    union_type = *(int *)((uint8_t *)ptr + field.offset);
                }
                if (level || in_array || schema_atom_present(field, (uint8_t *)ptr + field.offset)) {
                    indent(f, level + 1);
                    fprintf(f, "%s: ", field.name);
                    schema_atom_print(f, field, (uint8_t *)ptr + field.offset);
                    if (in_array) {
                        fprintf(f, ",");
                    }
                    if (field.type == TYP_ATOM_ENUM && field.enum_converter) {
                        int enum_type = *(int *)((uint8_t *)ptr + field.offset);
                        const char *enum_str = field.enum_converter(enum_type);
                        fprintf(f, "  # %s", enum_str);
                    }
                    fprintf(f, "\n");
                }
            }
        }
    }
}
void schema_atom_print_json(FILE *f, SchemaField &field, void *ptr)
{
    switch (field.type) {
        case TYP_ATOM_BOOL: {
            bool *val = (bool *)ptr;
            fprintf(f, "%s", *val ? "true" : "false");
            break;
        } case TYP_ATOM_UINT8: {
            uint8_t *val = (uint8_t *)ptr;
            fprintf(f, "%u", *val);
            break;
        } case TYP_ATOM_INT: {
            int32_t *val = (int32_t *)ptr;
            fprintf(f, "%d", *val);
            break;
        } case TYP_ATOM_UINT: {
            uint32_t *val = (uint32_t *)ptr;
            fprintf(f, "%u", *val);
            break;
        } case TYP_ATOM_FLOAT: {
            float *val = (float *)ptr;
            fprintf(f, "\"0x%08X (%f)\"", *(uint32_t *)val, *val);
            break;
        } case TYP_ATOM_STRING: {
            std::string *val = (std::string *)ptr;
            //fprintf(f, "\"%s\"  # %08X\n", IFNULL(*val, ""), (u32)*val);
            fprintf(f, "\"%s\"", val->c_str());
            break;
        } case TYP_ATOM_ENUM: {
            int *val = (int *)ptr;
            fprintf(f, "%d", *val);
            break;
        } default: {
            assert(!"Unexpected field type, don't know how to print");
        }
    }
}
void ta_schema_print_json(FILE *f, SchemaType type, void *ptr, int level, int in_array)
{
    static int last_char_comma = false;
    static fpos_t last_char_comma_pos = 0;

    Schema *schema = &g_schema_lib.schemas[type];

    // NOTE: Defaults to a value unlikely to be used in any legitimate enum to
    //       detect errors more easily.
    int union_type = -9001;

    size_t field_count = schema->fields.size();
    for (size_t field_idx = 0; field_idx < field_count; ++field_idx) {
        SchemaField &field = schema->fields[field_idx];
        if (field.is_alias) {
            continue;
        }
        if (field.in_union && field.union_type != union_type) {
            continue;
        }

        uint8_t *field_ptr = (uint8_t *)ptr + field.offset;

        if (field.array_len) {
            uint8_t *arr = field_ptr;
            size_t arr_len = field.array_len;
            if (arr_len) {
                if (arr_len == 1) {
                    arr = *(uint8_t **)field_ptr;
                    //arr_len = dlb_vec_len(arr);
                    assert(!"wtf is this shit");
                }
            }

            // Don't print empty arrays
            if (!arr_len) {
                //indent(f, level + 1);
                //fprintf(f, "\"%s\": []", field.name);
                continue;
            }

            indent(f, level + 1);
            fprintf(f, "\"%s\": [", field.name);
            for (size_t arr_idx = 0; arr_idx < arr_len; ++arr_idx) {
                if (field.type < TYP_COUNT) {
                    fprintf(f, "\n");
                    indent(f, level + 2);
                    fprintf(f, "{\n");
                    ta_schema_print_json(f, field.type, arr, level + 2, in_array + 1);
                    if (last_char_comma) {
                        assert(!fsetpos(f, &last_char_comma_pos));
                        fprintf(f, "\n");
                    }
                    indent(f, level + 2);
                    fprintf(f, "}");
                } else {
                    schema_atom_print_json(f, field, arr);
                }
                if (arr_len && arr_idx < arr_len - 1) {
                    assert(!fgetpos(f, &last_char_comma_pos));
                    fprintf(f, ",");
                    if (field.type > TYP_COUNT) {
                        fprintf(f, " ");
                    }
                    last_char_comma = true;
                } else {
                    last_char_comma = false;
                }
                if (field.type < TYP_COUNT) {
                    fprintf(f, "\n");
                }
                arr += field.size;
            }
            if (field.type < TYP_COUNT) {
                indent(f, level + 1);
            }
            assert(!last_char_comma);
            fprintf(f, "]");
        } else {
            if (field.type < TYP_COUNT) {
                indent(f, level + 1);
                fprintf(f, "\"%s\": {\n", field.name);
                ta_schema_print_json(f, field.type, field_ptr, level + 1, 0);
                if (last_char_comma) {
                    assert(!fsetpos(f, &last_char_comma_pos));
                    fprintf(f, "\n");
                }
                indent(f, level + 1);
                fprintf(f, "}");
            } else {
                if (field.is_union_type) {
                    union_type = *(int *)field_ptr;
                }
                if (level || in_array || schema_atom_present(field, field_ptr)) {
                    indent(f, level + 1);
                    fprintf(f, "\"%s\": ", field.name);
                    schema_atom_print_json(f, field, field_ptr);
                    fflush(f);
                    //if (field.type == TYP_ATOM_ENUM && field.enum_converter) {
                    //    int enum_type = *(int *)(ptr + field.offset);
                    //    const char *enum_str = field.enum_converter(enum_type);
                    //    fprintf(f, "  // %s", enum_str);
                    //}
                }
            }
        }

        if (field_count && field_idx < field_count - 1) {
            assert(!fgetpos(f, &last_char_comma_pos));
            fprintf(f, ",");
            last_char_comma = true;
        } else {
            last_char_comma = false;
        }
        fprintf(f, "\n");
    }
}