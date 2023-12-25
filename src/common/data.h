#pragma once
#include "common.h"
#include "strings.h"

struct Msg_S_TileChunk;
struct Msg_S_EntitySnapshot;
struct WangMap;

struct DrawCmd {
    Texture2D texture{};
    Rectangle rect{};
    Vector3   position{};
    Color     color{};

    bool operator<(const DrawCmd& rhs) const {
        const float me = position.y + position.z + rect.height;
        const float other = rhs.position.y + rhs.position.z + rhs.rect.height;
        return other < me;
    }
};
struct DrawCmdQueue : public std::priority_queue<DrawCmd> {
    void Draw(void) {
        while (!empty()) {
            const DrawCmd &cmd = top();
            const Vector2 pos = {
                cmd.position.x,
                cmd.position.y - cmd.position.z
            };
            dlb_DrawTextureRec(cmd.texture, cmd.rect, pos, cmd.color);
            //DrawCircle(pos.x, pos.y + cmd.rect.height, 3, PINK);
            pop();
        }
    }
};

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

//enum DataFlag : uint8_t {
//    DAT_FLAG_EMBEDDED = 0x1,
//    DAT_FLAG_EXTERNAL = 0x2,
//};

enum Direction : uint8_t {
    DIR_N,
    DIR_E,
    DIR_S,
    DIR_W,
    DIR_NE,
    DIR_SE,
    DIR_SW,
    DIR_NW
};

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

typedef uint8_t TileDefFlags;
enum {
    TILEDEF_FLAG_SOLID  = 0x01,
    TILEDEF_FLAG_LIQUID = 0x02,
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
    FIELD(uint16_t   , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
    HQT_GFX_FILE_FIELDS(HAQ_C_FIELD, 0);

    Texture texture{};
};

struct MusFile {
    static const DataType dtype = DAT_TYP_MUS_FILE;

#define HQT_MUS_FILE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
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
    FIELD(uint16_t                       , id            , {}, HAQ_SERIALIZE                                     , userdata) \
    FIELD(std::string                    , name          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(std::string                    , path          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(uint8_t                        , variations    , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(uint8_t                        , max_instances , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(float                          , pitch_variance, {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_FLOAT_HUNDRETH, userdata)
    HQT_SFX_FILE_FIELDS(HAQ_C_FIELD, 0);

    std::vector<SfxVariant> variants{};
};

struct GfxFrame {
    static const DataType dtype = DAT_TYP_GFX_FRAME;

#define HQT_GFX_FRAME_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id  , {}, HAQ_SERIALIZE                                      , userdata) \
    FIELD(std::string, name, {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(std::string, gfx , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(uint16_t   , x   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_X, userdata) \
    FIELD(uint16_t   , y   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_Y, userdata) \
    FIELD(uint16_t   , w   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(uint16_t   , h   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata)
    HQT_GFX_FRAME_FIELDS(HAQ_C_FIELD, 0);
};

struct GfxAnim {
    static const DataType dtype = DAT_TYP_GFX_ANIM;

#define HQT_GFX_ANIM_FIELDS(FIELD, userdata) \
    FIELD(uint16_t                , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string             , name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string             , sound      , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t                 , frame_count, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t                 , frame_delay, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::vector<std::string>, frames     , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
    HQT_GFX_ANIM_FIELDS(HAQ_C_FIELD, 0);

    bool soundPlayed{};

    const std::string &GetFrame(size_t index) const {
        if (index < frames.size()) {
            return frames[index];
        }
        return rnStringCatalog.Find(rnStringNull);
    }
};

struct Sprite {
    static const DataType dtype = DAT_TYP_SPRITE;

    using AnimArray = std::array<std::string, 8>;

#define HQT_SPRITE_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id   , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(AnimArray  , anims, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
    HQT_SPRITE_FIELDS(HAQ_C_FIELD, 0);
};

struct TileDef {
    static const DataType dtype = DAT_TYP_TILE_DEF;

#define HQT_TILE_DEF_FIELDS(FIELD, userdata) \
    FIELD(uint16_t    , id            , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string , name          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string , anim          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint16_t    , material_id   , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(TileDefFlags, flags         , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t     , auto_tile_mask, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
    HQT_TILE_DEF_FIELDS(HAQ_C_FIELD, 0);

    // color for minimap/wang tile editor (top left pixel of tile)
    Color        color      {};
    GfxAnimState anim_state {};
};

struct TileMat {
    static const DataType dtype = DAT_TYP_TILE_MAT;

#define HQT_TILE_MAT_FIELDS(FIELD, userdata) \
    FIELD(uint16_t   , id            , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, footstep_sound, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata)
    HQT_TILE_MAT_FIELDS(HAQ_C_FIELD, 0);
};

////////////////////////////////////////////////////////////////////////////

struct ObjectData {
    uint16_t x {};
    uint16_t y {};
    std::string type {};

    // type == "decoration"
    // no extra fields atm

    // type == "lever"
    uint8_t  power_level        {};
    uint16_t tile_def_unpowered {};
    uint16_t tile_def_powered   {};

    // type == "lootable"
    uint16_t loot_table_id {};

    // type == "sign"
    std::string sign_text[4] {};

    // type == "warp"
    uint16_t warp_map_id {};
    uint16_t warp_dest_x {};
    uint16_t warp_dest_y {};
    uint16_t warp_dest_z {};
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

struct TileChunk {
    uint16_t tile_ids   [SV_MAX_TILE_CHUNK_WIDTH * SV_MAX_TILE_CHUNK_WIDTH]{};  // TODO: Compress, use less bits, etc.
    uint16_t object_ids [SV_MAX_TILE_CHUNK_WIDTH * SV_MAX_TILE_CHUNK_WIDTH]{};  // TODO: Compress, use less bits, etc.
};

struct Tilemap {
    struct Coord {
        uint16_t x, y;

        bool operator==(const Coord &other) const
        {
            return x == other.x && y == other.y;
        }

        struct Hasher {
            size_t operator()(const Coord &coord) const
            {
                size_t hash = 0;
                hash_combine(hash, coord.x, coord.y);
                return hash;
            }
        };
    };
    typedef std::unordered_set<Coord, Coord::Hasher> CoordSet;

    static const DataType dtype = DAT_TYP_TILE_MAP;
    static const uint32_t MAGIC = 0xDBBB9192;
    static const uint16_t VERSION = 9;
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    // v5: added warps
    // v6: added auto-tile mask to tileDef
    // v7: tileDefCount no longer based on texture size, in case texture is moved/deleted
    // v8: add sentinel
    // v9: Vector3 path nodes

#define HQT_TILE_MAP_FIELDS(FIELD, userdata) \
    FIELD(uint16_t               , version    , {}, HAQ_SERIALIZE           , userdata) /* version on disk                */ \
    FIELD(uint16_t               , id         , {}, HAQ_SERIALIZE           , userdata) /* id of the map (for networking) */ \
    FIELD(std::string            , name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) /* name of map area               */ \
    FIELD(uint16_t               , width      , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) /* width of map in tiles          */ \
    FIELD(uint16_t               , height     , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) /* height of map in tiles         */ \
    FIELD(std::string            , title      , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) /* display name                   */ \
    FIELD(std::string            , bg_music   , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) /* background music               */ \
    FIELD(std::vector<uint16_t>  , tiles      , {}, HAQ_SERIALIZE/* | HAQ_EDIT */, userdata) \
    FIELD(std::vector<uint16_t>  , objects    , {}, HAQ_SERIALIZE/* | HAQ_EDIT */, userdata) \
    FIELD(std::vector<ObjectData>, object_data, {}, HAQ_SERIALIZE | HAQ_EDIT     , userdata) \
    FIELD(std::vector<AiPathNode>, path_nodes , {}, HAQ_SERIALIZE/* | HAQ_EDIT */, userdata) \
    FIELD(std::vector<AiPath>    , paths      , {}, HAQ_SERIALIZE/* | HAQ_EDIT */, userdata)
    HQT_TILE_MAP_FIELDS(HAQ_C_FIELD, 0);

    //-------------------------------
    // Not serialized
    //-------------------------------
    //uint16_t net_id             {};  // for communicating efficiently w/ client about which map
    double      chunkLastUpdatedAt {};  // used by server to know when chunks are dirty on clients
    CoordSet    dirtyTiles         {};  // tiles that have changed since last snapshot was sent
    Edge::Array edges              {};  // collision edge list

    //-------------------------------
    // Clean this section up
    //-------------------------------
    // Tiles
    uint16_t At(uint16_t x, uint16_t y);
    uint16_t At_Obj(uint16_t x, uint16_t y);
    bool AtTry(uint16_t x, uint16_t y, uint16_t &tile_id);
    bool AtTry_Obj(uint16_t x, uint16_t y, uint16_t &obj_id);
    bool WorldToTileIndex(uint16_t world_x, uint16_t world_y, Coord &coord);
    bool AtWorld(uint16_t world_x, uint16_t world_y, uint16_t &tile_id);
    bool IsSolid(int x, int y);  // tile x,y coord, returns true if out of bounds

    void Set(uint16_t x, uint16_t y, uint16_t tile_id, double now);
    void Set_Obj(uint16_t x, uint16_t y, uint16_t object_id, double now);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(uint16_t x, uint16_t y, uint16_t new_tile_id, double now);

    TileDef &GetTileDef(uint16_t tile_id);
    const GfxFrame &GetTileGfxFrame(uint16_t tile_id);
    Rectangle TileDefRect(uint16_t tile_id);
    Color TileDefAvgColor(uint16_t tile_id);

    // Objects
    ObjectData *GetObjectData(uint16_t x, uint16_t y);

    AiPath *GetPath(uint16_t pathId);
    uint16_t GetNextPathNodeIndex(uint16_t pathId, uint16_t pathNodeIndex);
    AiPathNode *GetPathNode(uint16_t pathId, uint16_t pathNodeIndex);

    void UpdateEdges(void);
    void ResolveEntityCollisionsEdges(Entity &entity);
    void ResolveEntityCollisionsTriggers(Entity &entity);

    void DrawTile(uint16_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws);
    void Draw(Camera2D &camera, DrawCmdQueue &sortedDraws);
    void DrawColliders(Camera2D &camera);
    void DrawEdges(void);
    void DrawTileIds(Camera2D &camera);

private:
    void GetEdges(Edge::Array &edges);

    bool NeedsFill(uint16_t x, uint16_t y, uint16_t old_tile_id);
    void Scan(uint16_t lx, uint16_t rx, uint16_t y, uint16_t old_tile_id, std::stack<Coord> &stack);
};

////////////////////////////////////////////////////////////////////////////

struct PackTocEntry {
    DataType dtype  {};
    int      offset {};  // byte offset in pack file binary data
    int      index  {};  // index of data in the in-memory vector

    PackTocEntry() = default;
    PackTocEntry(DataType dtype, int offset) : dtype(dtype), offset(offset) {}
};

struct PackToc {
    std::vector<PackTocEntry> entries;
};

// Inspired by https://twitter.com/angealbertini/status/1340712669247119360
struct Pack {
    std::string path       {};
    int         magic      {};
    int         version    {};
    int         toc_offset {};

    // static resources
    // - textures
    // - sounds
    // - music
    // - sprite defs
    // - tile defs
    // - object defs (TODO)
    std::vector<GfxFile>  gfx_files  {};
    std::vector<MusFile>  mus_files  {};
    std::vector<SfxFile>  sfx_files  {};
    std::vector<GfxFrame> gfx_frames {};
    std::vector<GfxAnim>  gfx_anims  {};
    std::vector<Sprite>   sprites    {};
    std::vector<TileDef>  tile_defs  {};
    std::vector<TileMat>  tile_mats  {};

    // static entities? (objects?)
    // - doors
    // - chests
    // - fireplaces
    // - workbenches
    // - flowers
    // - water

    // dynamic entities
    // - players
    // - townfolk
    // - pets
    // - fauna
    // - monsters
    //   - apparition
    //   - phantom
    //   - shade
    //   - shadow
    //   - soul
    //   - specter
    //   - umbra
    //   - sylph
    //   - vision
    //   - wraith
    // - particles? maybe?
    std::vector<Tilemap> tile_maps{};
    std::vector<Entity> entities{};

    std::unordered_map<uint16_t, size_t> dat_by_id[DAT_TYP_COUNT]{};
    std::unordered_map<std::string, size_t> dat_by_name[DAT_TYP_COUNT]{};

    PackToc toc {};

    Pack(const std::string &path) : path(path) {}

    void *GetPool(DataType dtype)
    {
        switch (dtype) {
            case DAT_TYP_GFX_FILE : return (void *)&gfx_files;
            case DAT_TYP_MUS_FILE : return (void *)&mus_files;
            case DAT_TYP_SFX_FILE : return (void *)&sfx_files;
            case DAT_TYP_GFX_FRAME: return (void *)&gfx_frames;
            case DAT_TYP_GFX_ANIM : return (void *)&gfx_anims;
            case DAT_TYP_SPRITE   : return (void *)&sprites;
            case DAT_TYP_TILE_DEF : return (void *)&tile_defs;
            case DAT_TYP_TILE_MAT : return (void *)&tile_mats;
            case DAT_TYP_TILE_MAP : return (void *)&tile_maps;
            case DAT_TYP_ENTITY   : return (void *)&entities;
            default: assert(!"that ain't a valid dtype, bruv"); return 0;
        }
    }

    template <typename T>
    T &FindById(uint16_t id) {
        auto &vec = *(std::vector<T> *)GetPool(T::dtype);
        const auto &map = dat_by_id[T::dtype];
        const auto &entry = map.find(id);
        if (entry != map.end()) {
            return vec[entry->second];
        } else {
            //TraceLog(LOG_WARNING, "Could not find resource of type %s by id '%u'.", DataTypeStr(T::dtype), id);
            return vec[0];
        }
    }

    template <typename T>
    T &FindByName(const std::string &name) {
        const DataType dtype = T::dtype;
        auto &vec = *(std::vector<T> *)GetPool(T::dtype);
        const auto &map = dat_by_name[dtype];
        const auto &entry = map.find(name);
        if (entry != map.end()) {
            return vec[entry->second];
        } else {
            //TraceLog(LOG_WARNING, "Could not find resource of type %s by name '%s'.", DataTypeStr(T::dtype), name.c_str());
            return vec[0];
        }
    }
};

enum PackStreamType {
    PACK_TYPE_BINARY,
    PACK_TYPE_TEXT
};

enum PackStreamMode {
    PACK_MODE_READ,
    PACK_MODE_WRITE,
};

typedef size_t (*ProcessFn)(void *buffer, size_t size, size_t count, FILE* stream);

struct PackStream {
    Pack *         pack    {};
    FILE *         file    {};
    PackStreamMode mode    {};
    PackStreamType type    {};
    ProcessFn      process {};

    PackStream(Pack *pack, FILE *file, PackStreamMode mode, PackStreamType type)
        : type(type), mode(mode), file(file), pack(pack)
    {
        if (mode == PACK_MODE_READ) {
            process = (ProcessFn)fread;
        } else {
            process = (ProcessFn)fwrite;
        }
    };
};

Err Init(void);
void Free(void);

Err SavePack(Pack &pack, PackStreamType type);
Err LoadPack(Pack &pack, PackStreamType type, bool loadResources = true);
void UnloadPack(Pack &pack);

void PlaySound(const std::string &id, float pitchVariance = 0.0f);
bool IsSoundPlaying(const std::string &id);
void StopSound(const std::string &id);

void UpdateSprite(Entity &entity, double dt, bool newlySpawned);
void ResetSprite(Entity &entity);
void DrawSprite(const Entity &entity, DrawCmdQueue *sortedDraws, bool highlight = false);

void UpdateTileDefAnimations(double dt);

const char *DataTypeStr(DataType type);
const char *EntityTypeStr(EntityType type);
const char *EntitySpeciesStr(EntitySpecies type);

// Assets
extern std::vector<Pack> packs;

// Shaders
extern Shader shdSdfText;
extern Shader shdPixelFixer;
extern int    shdPixelFixerScreenSizeUniformLoc;

// Fonts
extern Font fntTiny;
extern Font fntSmall;
extern Font fntMedium;
extern Font fntBig;