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
    HAQ_NONE                 = 0b00000000,  // no flags set
    HAQ_SERIALIZE            = 0b00000001,  // serialize field to pack file
    HAQ_EDIT                 = 0b00000010,  // allow field to be edited in editor (if type is supported)
    HAQ_EDIT_FLOAT_TENTH     = 0b00000100,  // set format to %.1f and increment to 0.1f
    HAQ_EDIT_FLOAT_HUNDRETH  = 0b00001000,  // set format to %.2f and increment to 0.01f
    HAQ_EDIT_TEXTBOX_STYLE_X = 0b00010000,  // set textbox background color to red
    HAQ_EDIT_TEXTBOX_STYLE_Y = 0b00100000,  // set textbox background color to green
    HAQ_EDIT_TEXTBOX_STYLE_Z = 0b01000000,  // set textbox background color to blue
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
    FIELD(uint16_t                       , id            , {}, HAQ_SERIALIZE                                     , true, userdata) \
    FIELD(std::string                    , name          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , true, userdata) \
    FIELD(std::string                    , path          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , true, userdata) \
    FIELD(uint8_t                        , variations    , {}, HAQ_SERIALIZE | HAQ_EDIT                          , true, userdata) \
    FIELD(uint8_t                        , max_instances , {}, HAQ_SERIALIZE | HAQ_EDIT                          , true, userdata) \
    FIELD(float                          , pitch_variance, {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_FLOAT_HUNDRETH, true, userdata)
    HQT_SFX_FILE_FIELDS(HAQ_C_FIELD, 0);

    std::vector<SfxVariant> variants{};
};

struct GfxFrame {
    static const DataType dtype = DAT_TYP_GFX_FRAME;

#define HQT_GFX_FRAME_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id  , {}, HAQ_SERIALIZE                                      , true, userdata) \
    FIELD(std::string, name, {}, HAQ_SERIALIZE | HAQ_EDIT                           , true, userdata) \
    FIELD(std::string, gfx , {}, HAQ_SERIALIZE | HAQ_EDIT                           , true, userdata) \
    FIELD(uint16_t   , x   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_X, true, userdata) \
    FIELD(uint16_t   , y   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_Y, true, userdata) \
    FIELD(uint16_t   , w   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , true, userdata) \
    FIELD(uint16_t   , h   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , true, userdata)
    HQT_GFX_FRAME_FIELDS(HAQ_C_FIELD, 0);
};

struct GfxAnim {
    static const DataType dtype = DAT_TYP_GFX_ANIM;

#define HQT_GFX_ANIM_FIELDS(FIELD, userdata) \
    FIELD(uint16_t                , id         , {}, HAQ_SERIALIZE           , true, userdata) \
    FIELD(std::string             , name       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(std::string             , sound      , {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
    FIELD(uint8_t                 , frame_count, {}, HAQ_SERIALIZE | HAQ_EDIT, true, userdata) \
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

struct ObjectData {
#define HQT_OBJECT_DATA_FIELDS(FIELD, udata)                                                                              \
    FIELD(uint16_t, x       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, udata)                                                  \
    FIELD(uint16_t, y       , {}, HAQ_SERIALIZE | HAQ_EDIT, true, udata)                                                  \
    FIELD(RNString, type    , {}, HAQ_SERIALIZE | HAQ_EDIT, true, udata)                                                  \
    FIELD(uint16_t, tile_def, {}, HAQ_SERIALIZE | HAQ_EDIT, true, udata)                                                  \
                                                                                                                          \
    /* type == "decoration" */                                                                                            \
    /* no extra fields atm  */                                                                                            \
                                                                                                                          \
    /* type == ["lever", "door"] */                                                                                       \
    FIELD(uint8_t , power_level     , {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_LEVER || udata.type == S_DOOR, udata) \
    FIELD(uint8_t , power_channel   , {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_LEVER || udata.type == S_DOOR, udata) \
    FIELD(uint16_t, tile_def_powered, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_LEVER || udata.type == S_DOOR, udata) \
                                                                                                                          \
    /* type == "lootable" */                                                                                              \
    FIELD(uint16_t, loot_table_id, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_LOOTABLE, udata)                         \
                                                                                                                          \
    /* type == "sign" */                                                                                                  \
    FIELD(std::string, sign_text, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_SIGN, udata)                              \
                                                                                                                          \
    /* type == "warp" */                                                                                                  \
    FIELD(uint16_t, warp_map_id, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_WARP, udata)                               \
    FIELD(uint16_t, warp_dest_x, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_WARP, udata)                               \
    FIELD(uint16_t, warp_dest_y, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_WARP, udata)                               \
    FIELD(uint16_t, warp_dest_z, {}, HAQ_SERIALIZE | HAQ_EDIT, udata.type == S_WARP, udata)
    HQT_OBJECT_DATA_FIELDS(HAQ_C_FIELD, 0);
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

// Assets
extern std::vector<Pack> packs;