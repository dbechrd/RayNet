#pragma once

#define SCHEMA_TYPES(gen) \
    gen(TYP_NULL)         \
    gen(TYP_VEC2)         \
    gen(TYP_VEC3)         \
    gen(TYP_RECT)         \
    gen(TYP_DAT_BUFFER)   \
    gen(TYP_GFX_FILE)     \
    gen(TYP_MUS_FILE)     \
    gen(TYP_SFX_FILE)     \
    gen(TYP_GFX_FRAME)    \
    gen(TYP_GFX_ANIM)     \
    gen(TYP_SPRITE)       \
    gen(TYP_TILE_MAT)     \
    gen(TYP_TILE_DEF)     \
    gen(TYP_ENTITY)       \
    gen(TYP_TILE_MAP)     \
    gen(TYP_FOO)          \
    /* gen(TYP_FONT)    */\
    /* gen(TYP_SHADER)  */\
    /* gen(TYP_TEXTURE) */\
    gen(TYP_COUNT)        \
    gen(TYP_ATOM_BOOL)    \
    gen(TYP_ATOM_UINT8)   \
    gen(TYP_ATOM_INT)     \
    gen(TYP_ATOM_UINT)    \
    gen(TYP_ATOM_FLOAT)   \
    gen(TYP_ATOM_STRING)  \
    gen(TYP_ATOM_ENUM)    \

enum SchemaType {
    SCHEMA_TYPES(ENUM_V_VALUE)
};

typedef const char *(EnumConverter)(int);

struct SchemaField {
    SchemaType    type;             // field type
    const char    *name;            // field name
    size_t        offset;           // field offset in the struct the parent schema represents
    size_t        size;             // field size in the struct
    size_t        array_len;        // 0 = not array, 1 = vector, >1 = fixed array size
    bool          is_alias;         // true if this field is just an alias for another field
    EnumConverter *enum_converter;  // enum to string converter (for enum fields)
    bool          is_union_type;    // true if this field is a union
    bool          in_union;         // true if this field is in a union
    int           union_type;       // enum representing the type for which this union field is associated (used to
                                    // validate union fields are only present when they match the field type)
};

typedef void (SchemaInit)(void *ptr);
typedef void (SchemaFree)(void *ptr);

struct Schema {
    SchemaType               type;    // field type of the top-most node representing this schema
    const char               *name;   // field name of the top-most node (i.e. "schema name")
    size_t                   size;    // size in bytes of the schema type and all of its children
    SchemaInit               *init;   // constructor
    SchemaFree               *free;   // destructor
    std::vector<SchemaField> fields;  // array of fields in this schema type
};

struct SchemaLibrary {
    Schema schemas[TYP_COUNT];
    std::unordered_map<std::string, Schema *> schemas_by_name;
};

extern SchemaLibrary g_schema_lib;

void          ta_schema_register       (void);
Schema      * ta_schema_find_by_name   (const std::string &name);
SchemaField * ta_schema_field_find     (SchemaType type, const std::string &name);
void          ta_schema_print          (FILE *f, SchemaType type, void *ptr, int level, int in_array);
void          ta_schema_print_json     (FILE *f, SchemaType type, void *ptr, int level, int in_array);
