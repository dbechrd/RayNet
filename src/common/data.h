#pragma once
#include "common.h"

// DO NOT re-order these! They are hard-coded in the TOC
#define DATA_TYPES(gen) \
    gen(DAT_TYP_GFX_FILE,  "GFXFILE ") \
    gen(DAT_TYP_MUS_FILE,  "MUSFILE ") \
    gen(DAT_TYP_SFX_FILE,  "SFXFILE ") \
    gen(DAT_TYP_GFX_FRAME, "GFXFRAME") \
    gen(DAT_TYP_GFX_ANIM,  "GFXANIM ") \
    gen(DAT_TYP_SPRITE,    "SPRITE  ") \
    gen(DAT_TYP_TILE_DEF,  "TILEDEF ") \
    gen(DAT_TYP_TILE_MAT,  "TILEMAT ") \
    gen(DAT_TYP_TILE_MAP,  "TILEMAP ") \
    gen(DAT_TYP_ENTITY,    "ENTITY  ")

enum DataType : uint8_t {
    DATA_TYPES(ENUM_VD_VALUE)
    DAT_TYP_COUNT,
    DAT_TYP_INVALID
};

const char *DataTypeStr(DataType type);

//enum DataFlag : uint8_t {
//    DAT_FLAG_EMBEDDED = 0x1,
//    DAT_FLAG_EXTERNAL = 0x2,
//};

#if 0
struct DatBuffer {
    bool filename;  // if true, the buffer contains a filename not the file data
    size_t length;
    uint8_t *bytes;
};

void ReadFileIntoDataBuffer(const std::string &filename, DatBuffer &datBuffer);
void FreeDataBuffer(DatBuffer &datBuffer);
#endif

struct GfxAnimState {
    uint8_t frame {};  // current frame index
    double  accum {};  // time since last update
};

enum HAQ_FLAGS {
    HAQ_NONE                    = 0b0000000000000000,  // no flags set
    HAQ_SERIALIZE               = 0b0000000000000001,  // serialize field to pack file
    HAQ_EDIT                    = 0b0000000000000010,  // allow field to be edited in editor (if type is supported)
    HAQ_STYLE_FLOAT_TENTH       = 0b0000000000000100,  // set format to %.1f and increment to 0.1f
    HAQ_STYLE_FLOAT_HUNDRETH    = 0b0000000000001000,  // set format to %.2f and increment to 0.01f
    HAQ_STYLE_STRING_MULTILINE  = 0b0000000000010000,  // allow multi-line editing (field must be std::string!)
    HAQ_STYLE_BGCOLOR_RED       = 0b0000000000100000,  // set textbox background color to red
    HAQ_STYLE_BGCOLOR_GREEN     = 0b0000000001000000,  // set textbox background color to green
    HAQ_STYLE_BGCOLOR_BLUE      = 0b0000000010000000,  // set textbox background color to blue
    HAQ_HOVER_SHOW_TILE         = 0b0000000100000000,  // show value as a tile texture on mouse hover

    // Some presets for convenience
    HAQ_EDIT_FLOAT_TENTH      = HAQ_EDIT | HAQ_STYLE_FLOAT_TENTH     ,
    HAQ_EDIT_FLOAT_HUNDRETH   = HAQ_EDIT | HAQ_STYLE_FLOAT_HUNDRETH  ,
    HAQ_EDIT_STRING_MULTILINE = HAQ_EDIT | HAQ_STYLE_STRING_MULTILINE,
    HAQ_EDIT_FLOAT_X          = HAQ_EDIT | HAQ_STYLE_BGCOLOR_RED     ,
    HAQ_EDIT_FLOAT_Y          = HAQ_EDIT | HAQ_STYLE_BGCOLOR_GREEN   ,
    HAQ_EDIT_FLOAT_Z          = HAQ_EDIT | HAQ_STYLE_BGCOLOR_BLUE    ,
    HAQ_EDIT_TILE_ID          = HAQ_EDIT | HAQ_HOVER_SHOW_TILE       ,
};

struct GfxFile {
    static const DataType dtype = DAT_TYP_GFX_FILE;

#define HQT_GFX_FILE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id         , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_GFX_FILE_FIELDS(HAQ_C_FIELD, 0);

    Texture texture{};
};

struct MusFile {
    static const DataType dtype = DAT_TYP_MUS_FILE;

#define HQT_MUS_FILE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id         , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_MUS_FILE_FIELDS(HAQ_C_FIELD, 0);

    Music music {};
};

struct SfxVariant {
    std::string        path      {};
    Sound              sound     {};
    std::vector<Sound> instances {};
};

struct SfxFile {
    static const DataType dtype = DAT_TYP_SFX_FILE;

#define HQT_SFX_FILE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t                       , id            , {}, HAQ_SERIALIZE                          , true, userdata) \
    FIELD(std::string                    , name          , {}, HAQ_SERIALIZE | HAQ_EDIT               , true, userdata) \
    FIELD(std::string                    , path          , {}, HAQ_SERIALIZE | HAQ_EDIT               , true, userdata) \
    FIELD(uint8_t                        , variations    , {}, HAQ_SERIALIZE | HAQ_EDIT               , true, userdata) \
    FIELD(uint8_t                        , max_instances , {}, HAQ_SERIALIZE | HAQ_EDIT               , true, userdata) \
    FIELD(float                          , pitch_variance, {}, HAQ_SERIALIZE | HAQ_EDIT_FLOAT_HUNDRETH, true, userdata)
    HQT_SFX_FILE_FIELDS(HAQ_C_FIELD, 0);

    std::vector<SfxVariant> variants{};
};

struct GfxFrame {
    static const DataType dtype = DAT_TYP_GFX_FRAME;

#define HQT_GFX_FRAME_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id  , {}, HAQ_SERIALIZE                   , true, userdata) \
    FIELD(std::string, name, {}, HAQ_SERIALIZE | HAQ_EDIT        , true, userdata) \
    FIELD(std::string, gfx , {}, HAQ_SERIALIZE | HAQ_EDIT        , true, userdata) \
    FIELD(uint16_t   , x   , {}, HAQ_SERIALIZE | HAQ_EDIT_FLOAT_X, true, userdata) \
    FIELD(uint16_t   , y   , {}, HAQ_SERIALIZE | HAQ_EDIT_FLOAT_Y, true, userdata) \
    FIELD(uint16_t   , w   , {}, HAQ_SERIALIZE | HAQ_EDIT        , true, userdata) \
    FIELD(uint16_t   , h   , {}, HAQ_SERIALIZE | HAQ_EDIT        , true, userdata)
    HQT_GFX_FRAME_FIELDS(HAQ_C_FIELD, 0);
};

struct GfxAnim {
    static const DataType dtype = DAT_TYP_GFX_ANIM;

#define HQT_GFX_ANIM_FIELDS(FIELD, userdata) \
    FIELD(uint16_t                , id         , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string             , name       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string             , sound      , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    /*FIELD(uint8_t                 , frame_count, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)*/ \
    FIELD(uint8_t                 , frame_delay, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::vector<std::string>, frames     , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_GFX_ANIM_FIELDS(HAQ_C_FIELD, 0);

    bool soundPlayed{};

    const std::string &GetFrame(size_t index) const {
        if (index < frames.size()) {
            return frames[index];
        }
        return rnStringCatalog.Find(S_NULL);
    }
};

struct Sprite {
    static const DataType dtype = DAT_TYP_SPRITE;

    using AnimArray = std::array<std::string, 8>;

#define HQT_SPRITE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id   , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string, name , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(AnimArray  , anims, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_SPRITE_FIELDS(HAQ_C_FIELD, 0);
};

struct TileDef {
    static const DataType dtype = DAT_TYP_TILE_DEF;

    enum Flags {
        FLAG_SOLID  = 0x01,
        FLAG_LIQUID = 0x02,
    };

#define HQT_TILE_DEF_FIELDS(FIELD, userdata) \
    FIELD(uint16_t    , id            , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string , name          , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string , anim          , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(uint16_t    , material_id   , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(Flags       , flags         , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(uint8_t     , auto_tile_mask, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_TILE_DEF_FIELDS(HAQ_C_FIELD, 0);

    // color for minimap/wang tile editor (top left pixel of tile)
    Color        color      {};
    GfxAnimState anim_state {};
};

struct TileMat {
    static const DataType dtype = DAT_TYP_TILE_MAT;

#define HQT_TILE_MAT_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id            , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string, name          , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string, footstep_sound, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata)
    HQT_TILE_MAT_FIELDS(HAQ_C_FIELD, 0);
};

////////////////////////////////////////////////////////////////////////////

// DO NOT re-order these! They are hard-coded in pack files
#define OBJ_TYPES(gen) \
    /*  type          name        tile_def_id */ \
    gen(OBJ_NONE    , "<none>"  , 0            ) \
    gen(OBJ_DECOR   , "Decor"   , 0            ) \
    gen(OBJ_LEVER   , "Lever"   , 19           ) \
    gen(OBJ_DOOR    , "Door"    , 21           ) \
    gen(OBJ_LOOTABLE, "Lootable", 16           ) \
    gen(OBJ_SIGN    , "Sign"    , 18           ) \
    gen(OBJ_WARP    , "Warp"    , 17           ) \
    gen(OBJ_COUNT   , "<count>" , 0            )

enum ObjType {
    OBJ_TYPES(ENUM_VDM_VALUE)
};

const char *ObjTypeStr(ObjType type);
uint16_t ObjTypeTileDefId(ObjType type);

struct ObjectData {
#define HQT_OBJECT_DATA_FIELDS(FIELD, udata)                                                                                       \
    FIELD(ObjType , type, {}, HAQ_SERIALIZE | HAQ_EDIT        , true, udata)                                                       \
    FIELD(uint16_t, x   , {}, HAQ_SERIALIZE | HAQ_EDIT        , true, udata)                                                       \
    FIELD(uint16_t, y   , {}, HAQ_SERIALIZE | HAQ_EDIT        , true, udata)                                                       \
    FIELD(uint16_t, tile, {}, HAQ_SERIALIZE | HAQ_EDIT_TILE_ID, true, udata)                                                       \
                                                                                                                                   \
    /* type == "decoration" */                                                                                                     \
    /* no extra fields atm  */                                                                                                     \
                                                                                                                                   \
    /* type == ["lever", "door"] */                                                                                                \
    FIELD(uint16_t, tile_powered , {}, HAQ_SERIALIZE | HAQ_EDIT_TILE_ID, udata.type == OBJ_LEVER || udata.type == OBJ_DOOR, udata) \
    FIELD(uint8_t , power_level  , {}, HAQ_SERIALIZE | HAQ_EDIT        , udata.type == OBJ_LEVER || udata.type == OBJ_DOOR, udata) \
    FIELD(uint8_t , power_channel, {}, HAQ_SERIALIZE | HAQ_EDIT        , udata.type == OBJ_LEVER || udata.type == OBJ_DOOR, udata) \
                                                                                                                                   \
    /* type == "lootable" */                                                                                                       \
    FIELD(uint16_t, loot_table_id, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == OBJ_LOOTABLE, udata)                                \
                                                                                                                                   \
    /* type == "sign" */                                                                                                           \
    FIELD(std::string, sign_text, {}, HAQ_SERIALIZE | HAQ_EDIT_STRING_MULTILINE, udata.type == OBJ_SIGN, udata)                    \
                                                                                                                                   \
    /* type == "warp" */                                                                                                           \
    FIELD(uint16_t, warp_map_id, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == OBJ_WARP, udata)                                      \
    FIELD(uint16_t, warp_dest_x, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == OBJ_WARP, udata)                                      \
    FIELD(uint16_t, warp_dest_y, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == OBJ_WARP, udata)                                      \
    FIELD(uint16_t, warp_dest_z, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == OBJ_WARP, udata)
    HQT_OBJECT_DATA_FIELDS(HAQ_C_FIELD, 0);
#if 0
    static RNString ObjTypeToStr(ObjType type)
    {
        switch (type) {
            case OBJ_DECOR    : return S_DECOR   ;
            case OBJ_LEVER    : return S_LEVER   ;
            case OBJ_DOOR     : return S_DOOR    ;
            case OBJ_LOOTABLE : return S_LOOTABLE;
            case OBJ_SIGN     : return S_SIGN    ;
            case OBJ_WARP     : return S_WARP    ;
        }
    }

    static ObjType ObjTypeFromStr(RNString str)
    {
        switch (str.id) {
            case S_DECOR   .id : return OBJ_DECOR   ;
            case S_LEVER   .id : return OBJ_LEVER   ;
            case S_DOOR    .id : return OBJ_DOOR    ;
            case S_LOOTABLE.id : return OBJ_LOOTABLE;
            case S_SIGN    .id : return OBJ_SIGN    ;
            case S_WARP    .id : return OBJ_WARP    ;
        }
    }
#endif
};

struct AiPathNode {
    Vector3 pos;
    double waitFor;
};

struct AiPath {
    uint16_t pathNodeStart;
    uint16_t pathNodeCount;
};

#include "entity.h"
#include "tilemap.h"
#include "pack.h"

Err Init(void);
void Free(void);

void PlaySound(const std::string &id, float pitchVariance = 0.0f);
bool IsSoundPlaying(const std::string &id);
void StopSound(const std::string &id);

void UpdateSprite(Entity &entity, double dt, bool newlySpawned);
void ResetSprite(Entity &entity);
void DrawSprite(const Entity &entity, DrawCmdQueue *sortedDraws, bool highlight = false);

void UpdateTileDefAnimations(double dt);

// Shaders
extern Shader shdSdfText;
extern Shader shdPixelFixer;
extern int    shdPixelFixerScreenSizeUniformLoc;

// Fonts
extern Font fntTiny;
extern Font fntSmall;
extern Font fntMedium;
extern Font fntBig;

// Packs
extern Pack pack_assets;
extern Pack pack_maps;
