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

//enum DataFlag : uint32_t {
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

struct DatBuffer {
    bool filename;  // if true, the buffer contains a filename not the file data
    size_t length;
    uint8_t *bytes;
};

struct GfxAnimState {
    uint8_t frame {};  // current frame index
    double  accum {};  // time since last update
};

typedef uint32_t TileDefFlags;
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

#define HQT_GFX_FILE(TYPE, FIELD, OTHER, userdata) \
TYPE(struct, GfxFile, { \
    OTHER(static const DataType dtype = DAT_TYP_GFX_FILE;) \
    FIELD(uint32_t   , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(DatBuffer  , data_buffer, {}, HAQ_SERIALIZE           , userdata) \
    FIELD(Texture    , texture    , {}, HAQ_NONE                , userdata) \
}, userdata)

HAQ_C(HQT_GFX_FILE, 0);

#define HQT_MUS_FILE(TYPE, FIELD, OTHER, userdata) \
TYPE(struct, MusFile, { \
    OTHER(static const DataType dtype = DAT_TYP_MUS_FILE;) \
    FIELD(uint32_t   , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, path       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(DatBuffer  , data_buffer, {}, HAQ_SERIALIZE           , userdata) \
    FIELD(Music      , music      , {}, HAQ_NONE                , userdata) \
}, userdata)

HAQ_C(HQT_MUS_FILE, 0);

#define HQT_SFX_FILE(TYPE, FIELD, OTHER, userdata) \
TYPE(struct, SfxFile, { \
    OTHER(static const DataType dtype = DAT_TYP_SFX_FILE;) \
    FIELD(uint32_t          , id            , {}, HAQ_SERIALIZE                                     , userdata) \
    FIELD(std::string       , name          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(std::string       , path          , {}, HAQ_SERIALIZE | HAQ_EDIT                          , userdata) \
    FIELD(int               , variations    , {}, HAQ_SERIALIZE /*| HAQ_EDIT*/                      , userdata) \
    FIELD(float             , pitch_variance, {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_FLOAT_HUNDRETH, userdata) \
    FIELD(int               , max_instances , {}, HAQ_SERIALIZE /*| HAQ_EDIT*/                      , userdata) \
    FIELD(DatBuffer         , data_buffer   , {}, HAQ_SERIALIZE                                     , userdata) \
    FIELD(Sound             , sound         , {}, HAQ_NONE                                          , userdata) \
    FIELD(std::vector<Sound>, instances     , {}, HAQ_NONE                                          , userdata) \
}, userdata)

HAQ_C(HQT_SFX_FILE, 0);

#define HQT_GFX_FRAME(TYPE, FIELD, OTHER, userdata) \
TYPE(struct, GfxFrame, { \
    OTHER(static const DataType dtype = DAT_TYP_GFX_FRAME;) \
    FIELD(uint32_t   , id  , {}, HAQ_SERIALIZE                                      , userdata) \
    FIELD(std::string, name, {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(std::string, gfx , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(uint16_t   , x   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_X, userdata) \
    FIELD(uint16_t   , y   , {}, HAQ_SERIALIZE | HAQ_EDIT | HAQ_EDIT_TEXTBOX_STYLE_Y, userdata) \
    FIELD(uint16_t   , w   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
    FIELD(uint16_t   , h   , {}, HAQ_SERIALIZE | HAQ_EDIT                           , userdata) \
}, userdata)

HAQ_C(HQT_GFX_FRAME, 0);

#define HQT_GFX_ANIM(TYPE, FIELD, OTHER, userdata)                                       \
TYPE(struct, GfxAnim, {                                                                  \
    OTHER(static const DataType dtype = DAT_TYP_GFX_ANIM;)                               \
    FIELD(uint32_t                , id         , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string             , name       , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string             , sound      , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t                 , frame_rate , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t                 , frame_count, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t                 , frame_delay, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::vector<std::string>, frames     , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    OTHER(                                                                               \
        bool soundPlayed{};                                                              \
                                                                                         \
        const std::string &GetFrame(size_t index) const {                                \
            if (index < frames.size()) {                                                 \
                return frames[index];                                                    \
            }                                                                            \
            return rnStringCatalog.Find(rnStringNull);                                   \
        }                                                                                \
    )                                                                                    \
}, userdata)

HAQ_C(HQT_GFX_ANIM, 0);

#define HQT_SPRITE(TYPE, FIELD, OTHER, userdata)                                              \
TYPE(struct, Sprite, {                                                                        \
    OTHER(static const DataType dtype = DAT_TYP_SPRITE;)                                      \
    FIELD(uint32_t                           , id   , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string                        , name , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::array<std::string HAQ_COMMA 8>, anims, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
}, userdata)

HAQ_C(HQT_SPRITE, 0);

#define HQT_TILE_DEF(TYPE, FIELD, OTHER, userdata)                              \
TYPE(struct, TileDef, {                                                         \
    OTHER(static const DataType dtype = DAT_TYP_TILE_DEF;)                      \
    FIELD(uint32_t    , id            , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string , name          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string , anim          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint32_t    , material_id   , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(TileDefFlags, flags         , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(uint8_t     , auto_tile_mask, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    OTHER(                                                                      \
        /* color for minimap/wang tile editor (top left pixel of tile) */       \
        Color        color      {};                                             \
        GfxAnimState anim_state {};                                             \
    )                                                                           \
}, userdata)

HAQ_C(HQT_TILE_DEF, 0);

#define HQT_TILE_MAT(TYPE, FIELD, OTHER, userdata)                             \
TYPE(struct, TileMat, {                                                        \
    OTHER(static const DataType dtype = DAT_TYP_TILE_MAT;)                     \
    FIELD(uint32_t   , id            , {}, HAQ_SERIALIZE           , userdata) \
    FIELD(std::string, name          , {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
    FIELD(std::string, footstep_sound, {}, HAQ_SERIALIZE | HAQ_EDIT, userdata) \
}, userdata)

HAQ_C(HQT_TILE_MAT, 0);

////////////////////////////////////////////////////////////////////////////

struct ObjectData {
    uint32_t x {};
    uint32_t y {};
    std::string type {};

    // type == "decoration"
    // no extra fields atm

    // type == "lever"
    uint8_t  power_level        {};
    uint32_t tile_def_unpowered {};
    uint32_t tile_def_powered   {};

    // type == "lootable"
    uint32_t loot_table_id {};

    // type == "sign"
    std::string sign_text[4] {};

    // type == "warp"
    uint32_t warp_map_id {};
    uint32_t warp_dest_x {};
    uint32_t warp_dest_y {};
    uint32_t warp_dest_z {};
};

struct AiPathNode {
    Vector3 pos;
    double waitFor;
};

struct AiPath {
    uint32_t pathNodeStart;
    uint32_t pathNodeCount;
};

#include "entity.h"

struct TileChunk {
    uint32_t tile_ids   [SV_MAX_TILE_CHUNK_WIDTH * SV_MAX_TILE_CHUNK_WIDTH]{};  // TODO: Compress, use less bits, etc.
    uint32_t object_ids [SV_MAX_TILE_CHUNK_WIDTH * SV_MAX_TILE_CHUNK_WIDTH]{};  // TODO: Compress, use less bits, etc.
};

struct Tilemap {
    struct Coord {
        uint32_t x, y;

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
    // v1: the OG
    // v2: added texturePath
    // v3: added AI paths/nodes
    // v4: tileDefCount and tileDef.x/y are now implicit based on texture size
    // v5: added warps
    // v6: added auto-tile mask to tileDef
    // v7: tileDefCount no longer based on texture size, in case texture is moved/deleted
    // v8: add sentinel
    // v9: Vector3 path nodes
    static const uint32_t VERSION = 9;
    static const uint32_t SENTINEL = 0x12345678;

    uint32_t    version          {};  // version on disk
    uint32_t    id               {};  // id of the map (for networking)
    std::string name             {};  // name of map area
    uint32_t    width            {};  // width of map in tiles
    uint32_t    height           {};  // height of map in tiles
    std::string title            {};  // display name
    std::string background_music {};  // background music

    std::vector<uint32_t>   tiles       {};
    std::vector<uint32_t>   objects     {};
    //std::vector<bool>       solid       {};
    // TODO(dlb): Consider having vector<Warp>, vector<Lootable> etc. and having vector<ObjectData> be pointers/indices into those tables?
    std::vector<ObjectData> object_data {};
    std::vector<AiPathNode> pathNodes   {};  // 94 19 56 22 57
    std::vector<AiPath>     paths       {};  // offset, length | 0, 3 | 3, 3

    //-------------------------------
    // Not serialized
    //-------------------------------
    //uint32_t net_id             {};  // for communicating efficiently w/ client about which map
    double      chunkLastUpdatedAt {};  // used by server to know when chunks are dirty on clients
    CoordSet    dirtyTiles         {};  // tiles that have changed since last snapshot was sent
    Edge::Array edges              {};  // collision edge list

    //-------------------------------
    // Clean this section up
    //-------------------------------
    // Tiles
    uint32_t At(uint32_t x, uint32_t y);
    uint32_t At_Obj(uint32_t x, uint32_t y);
    bool AtTry(uint32_t x, uint32_t y, uint32_t &tile_id);
    bool AtTry_Obj(uint32_t x, uint32_t y, uint32_t &obj_id);
    bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
    bool AtWorld(uint32_t world_x, uint32_t world_y, uint32_t &tile_id);
    bool IsSolid(int x, int y);  // tile x,y coord, returns true if out of bounds

    void Set(uint32_t x, uint32_t y, uint32_t tile_id, double now);
    void Set_Obj(uint32_t x, uint32_t y, uint32_t object_id, double now);
    void SetFromWangMap(WangMap &wangMap, double now);
    void Fill(uint32_t x, uint32_t y, uint32_t new_tile_id, double now);

    TileDef &GetTileDef(uint32_t tile_id);
    const GfxFrame &GetTileGfxFrame(uint32_t tile_id);
    Rectangle TileDefRect(uint32_t tile_id);
    Color TileDefAvgColor(uint32_t tile_id);

    // Objects
    ObjectData *GetObjectData(uint32_t x, uint32_t y);

    AiPath *GetPath(uint32_t pathId);
    uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex);
    AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex);

    void UpdateEdges(void);
    void ResolveEntityCollisionsEdges(Entity &entity);
    void ResolveEntityCollisionsTriggers(Entity &entity);

    void DrawTile(uint32_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws);
    void Draw(Camera2D &camera, DrawCmdQueue &sortedDraws);
    void DrawColliders(Camera2D &camera);
    void DrawEdges(void);
    void DrawTileIds(Camera2D &camera);

private:
    void GetEdges(Edge::Array &edges);

    bool NeedsFill(uint32_t x, uint32_t y, uint32_t old_tile_id);
    void Scan(uint32_t lx, uint32_t rx, uint32_t y, uint32_t old_tile_id, std::stack<Coord> &stack);
};

// ECS test
struct foo {
    Vector3 dat{ 1, 2, 3 };
    SfxFile sfx{ 3, "sfx_id", "sfx.wav", 1, 0.05f, 8 };
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

    std::unordered_map<uint32_t, size_t> dat_by_id[DAT_TYP_COUNT]{};
    std::unordered_map<std::string, size_t> dat_by_name[DAT_TYP_COUNT]{};

    // TODO: Use a linked list, or something else that doesn't require a map of vectors when most vectors would be empty
    std::unordered_map<uint32_t, std::vector<size_t>> sfx_variants_by_id{};  // vector holds variants

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
    T &FindById(uint32_t id) {
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

void ReadFileIntoDataBuffer(const std::string &filename, DatBuffer &datBuffer);
void FreeDataBuffer(DatBuffer &datBuffer);

Err Init(void);
void Free(void);

Err SavePack(Pack &pack, PackStreamType type);
Err LoadPack(Pack &pack, PackStreamType type);
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