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
                                        \
    gen(DAT_TYP_GFX_FRAME, "GFXFRAME") \
    gen(DAT_TYP_GFX_ANIM,  "GFXANIM ") \
    gen(DAT_TYP_TILE_MAT,  "TILEMAT ") \
    gen(DAT_TYP_SPRITE,    "SPRITE  ") \
    gen(DAT_TYP_TILE_DEF,  "TILEDEF ") \
                                        \
    gen(DAT_TYP_TILE_MAP,  "TILEMAP ") \
    gen(DAT_TYP_ENTITY,    "ENTITY  ")

enum DataType : uint8_t {
    DATA_TYPES(ENUM_GEN_VALUE_DESC)
    DAT_TYP_COUNT
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

#if 0
#define GFX_FILE_IDS(gen)        \
gen(GFX_FILE_NONE)           \
gen(GFX_FILE_DLG_NPATCH)     \
gen(GFX_FILE_CHR_MAGE)       \
gen(GFX_FILE_NPC_LILY)       \
gen(GFX_FILE_NPC_FREYE)      \
gen(GFX_FILE_NPC_NESSA)      \
gen(GFX_FILE_NPC_ELANE)      \
gen(GFX_FILE_NPC_CHICKEN)    \
gen(GFX_FILE_OBJ_CAMPFIRE)   \
gen(GFX_FILE_PRJ_FIREBALL)   \
gen(GFX_FILE_TIL_OVERWORLD)  \
gen(GFX_FILE_TIL_AUTO_GRASS)

enum GfxFileId : uint16_t {
    GFX_FILE_IDS(ENUM_GEN_VALUE)
};
#endif

struct GfxFile {
    static const DataType dtype = DAT_TYP_GFX_FILE;
    std::string id          {};
    std::string path        {};
    DatBuffer   data_buffer {};
    ::Texture   texture     {};
};

struct MusFile {
    static const DataType dtype = DAT_TYP_MUS_FILE;
    std::string id          {};
    std::string path        {};
    DatBuffer   data_buffer {};
    ::Music     music       {};
};

struct SfxFile {
    static const DataType dtype = DAT_TYP_SFX_FILE;
    std::string          id             {};
    std::string          path           {};
    int                  variations     {};  // how many version of the file to look for with "_00" suffix
    float                pitch_variance {};
    int                  max_instances  {};  // how many can be played at the same time
    DatBuffer            data_buffer    {};

    // Runtime data
    ::Sound              sound          {};
    std::vector<::Sound> instances      {};  // "SoundAlias" in raylib, shares buffer, replaces PlaySoundMulti API
};

struct GfxFrame {
    static const DataType dtype = DAT_TYP_GFX_FRAME;
    std::string id  {};
    std::string gfx {};  // TODO: StringId
    uint16_t    x   {};
    uint16_t    y   {};
    uint16_t    w   {};
    uint16_t    h   {};
};

struct GfxAnim {
    static const DataType dtype = DAT_TYP_GFX_ANIM;
    std::string              id          {};
    std::string              sound       {};
    uint8_t                  frame_rate  {};
    uint8_t                  frame_count {};
    uint8_t                  frame_delay {};
    std::vector<std::string> frames      {};

    bool soundPlayed{};

    const std::string &GetFrame(size_t index) const {
        if (index < frames.size()) {
            return frames[index];
        }
        return rnStringCatalog.Find(rnStringNull);
    }
};

struct GfxAnimState {
    uint8_t frame {};  // current frame index
    double  accum {};  // time since last update
};

struct TileMat {
    static const DataType dtype = DAT_TYP_TILE_MAT;
    std::string   id             {};
    std::string   footstep_sound {};
};

//apparition
//phantom
//shade
//shadow
//soul
//specter
//spook
//sprite
//umbra
//sylph
//vision
//wraith
struct Sprite {
    static const DataType dtype = DAT_TYP_SPRITE;
    uint32_t    id       {};
    std::string name     {};
    std::string anims[8] {};  // for each direction
};

typedef uint32_t TileDefFlags;
enum {
    TILEDEF_FLAG_SOLID  = 0x01,
    TILEDEF_FLAG_LIQUID = 0x02,
};

struct TileDef {
    static const DataType dtype = DAT_TYP_TILE_DEF;
    uint32_t     id             {};
    std::string  name           {};
    std::string  anim           {};
    std::string  mat            {};
    TileDefFlags flags          {};
    uint8_t      auto_tile_mask {};  // not used atm, but this is where it would go once tiledefs are moved to tilesets

    //------------------------
    // Not serialized
    Color        color      {};  // color for minimap/wang tile editor (top left pixel of tile)
    GfxAnimState anim_state {};
};

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
    std::string loot_table_id {};

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

struct Entity;

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

    void GetEdges(Edge::Array &edges);
    void MergeEdges(Edge::Array &edges);
    void UpdateEdges(void);
    void ResolveEntityCollisionsEdges(Entity &entity);
    void ResolveEntityCollisions(Entity &entity);
    void ResolveEntityCollisionsTriggers(Entity &entity);

    void DrawTile(uint32_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws);
    void Draw(Camera2D &camera, DrawCmdQueue &sortedDraws);
    void DrawColliders(Camera2D &camera);
    void DrawEdges(void);
    void DrawTileIds(Camera2D &camera);

private:
    bool NeedsFill(uint32_t x, uint32_t y, uint32_t old_tile_id);
    void Scan(uint32_t lx, uint32_t rx, uint32_t y, uint32_t old_tile_id, std::stack<Coord> &stack);
};

////////////////////////////////////////////////////////////////////////////

// Best rap song: "i added it outta habit" by dandymcgee
#define ENTITY_TYPES(gen)  \
gen(ENTITY_NONE)       \
gen(ENTITY_PLAYER)     \
gen(ENTITY_NPC)        \
gen(ENTITY_OBJECT)     \
gen(ENTITY_PROJECTILE)

enum EntityType : uint8_t {
    ENTITY_TYPES(ENUM_GEN_VALUE)
};

#define ENTITY_SPECIES(gen)       \
gen(ENTITY_SPEC_NONE)         \
gen(ENTITY_SPEC_NPC_TOWNFOLK) \
gen(ENTITY_SPEC_NPC_CHICKEN)  \
gen(ENTITY_SPEC_OBJ_WARP)     \
gen(ENTITY_SPEC_PRJ_FIREBALL)

enum EntitySpecies : uint8_t {
    ENTITY_SPECIES(ENUM_GEN_VALUE)
};

struct EntityProto {
    EntityType    type                 {};
    EntitySpecies spec                 {};
    std::string   name                 {};
    std::string   ambient_fx           {};
    double        ambient_fx_delay_min {};
    double        ambient_fx_delay_max {};
    float         radius               {};
    std::string   dialog_root_key      {};
    float         hp_max               {};
    int           path_id              {};
    float         speed_min            {};
    float         speed_max            {};
    float         drag                 {};
    uint32_t      sprite_id            {};
    Direction     direction            {};
};

struct GhostSnapshot {
    double   server_time {};

    // Entity
    uint32_t map_id      {};
    Vector3  position    {};
    bool     on_warp_cooldown {};

    // Physics
    float    speed       {};  // max walk speed or something
    Vector3  velocity    {};

    // Life
    int      hp_max      {};
    int      hp          {};

    // TODO: Wtf do I do with this shit?
    uint32_t last_processed_input_cmd {};

    GhostSnapshot(void) {}
    GhostSnapshot(Msg_S_EntitySnapshot &msg);
};
typedef RingBuffer<GhostSnapshot, CL_SNAPSHOT_COUNT> AspectGhost;

// std::string -> StringId
// 672 bytes -> 296 bytes
struct Entity {
    static const DataType dtype = DAT_TYP_ENTITY;

    //// Entity ////
    uint32_t      id           {};
    EntityType    type         {};
    EntitySpecies spec         {};
    std::string   name         {};
    uint32_t      caused_by    {};  // who spawned the projectile?
    double        spawned_at   {};
    double        despawned_at {};
    AspectGhost   *ghost       {};

    uint32_t      map_id       {};
    Vector3       position     {};

    //// Audio ////
    std::string ambient_fx           {};  // some sound they play occasionally
    double      ambient_fx_delay_min {};
    double      ambient_fx_delay_max {};

    //// Collision ////
    float radius      {};  // collision
    bool  colliding   {};  // not sync'd, local flag for debugging colliders
    bool  on_warp     {};  // currently colliding with a warp
    struct {
        uint32_t x {};
        uint32_t y {};
    } on_warp_coord;  // coord of first detected warp we're standing on
    bool  on_warp_cooldown {};  // true when we've already warped but have not stepped off all warps yet (to prevent ping-ponging)

    //// Combat ////
    double last_attacked_at {};
    double attack_cooldown  {};

    //// Dialog ////
    // server-side
    std::string dialog_root_key   {};  // Root node of dialog tree

    // TODO: replace with a pointer to a pool of active dialogs (or just 1??)
    // client-side
    double      dialog_spawned_at {};  // time when dialog was spawned
    uint32_t    dialog_id         {};  // which dialog is active
    std::string dialog_title      {};  // name of NPC, submenu, etc.
    std::string dialog_message    {};  // what they're saying

    //// Inventory ////
    std::string holdingItem {};

    //// Life ////
    float hp_max    {};
    float hp        {};
    float hp_smooth {};  // client-only to smoothly interpolate health changes

    //// Pathfinding ////
    int    path_id                {};
    int    path_node_last_reached {};
    int    path_node_target       {};
    double path_node_arrived_at   {};

    Vector3 path_rand_direction  {};  // move this direction (if possible)
    double  path_rand_duration   {};  // for this long
    double  path_rand_started_at {};  // when we started moving this way

    //// Physics ////
    float   drag        {};
    float   speed       {};
    Vector3 force_accum {};
    Vector3 velocity    {};

    //// Sprite ////
    uint32_t     sprite_id  {};  // sprite resource
    Direction    direction  {};  // current facing direction
    GfxAnimState anim_state {};  // keep track of the animation as it plays

    //// Warp ////
    Rectangle warp_collider {};
    Vector3   warp_dest_pos {};

    // You either need this
    uint32_t    warp_dest_map         {};  // regular map to warp to
    // Or both of these
    uint32_t    warp_template_map     {};  // template map to make a copy of for procgen
    std::string warp_template_tileset {};  // wang tileset to use for procgen

    Vector2 Position2D(void);
    const GfxFrame &GetSpriteFrame() const;
    Rectangle GetSpriteRect() const;
    Vector2 TopCenter(void) const;
    void ClearDialog(void);
    void TakeDamage(int damage);
    bool Alive(void);
    bool Dead(void);
    void ApplyForce(Vector3 force);
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
    std::vector<TileMat>  tile_mats  {};
    std::vector<Sprite>   sprites    {};
    std::vector<TileDef>  tile_defs  {};

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
    // - particles? maybe?
    std::vector<Entity> entities{};
    std::vector<Tilemap> tile_maps{};

    std::unordered_map<std::string, size_t> gfx_file_by_id{};
    std::unordered_map<std::string, size_t> mus_file_by_id{};
    std::unordered_map<std::string, std::vector<size_t>> sfx_file_by_id{};  // vector holds variants

    std::unordered_map<std::string, size_t> gfx_frame_by_id{};
    std::unordered_map<std::string, size_t> gfx_anim_by_id{};
    std::unordered_map<std::string, size_t> tile_mat_by_id{};
    std::unordered_map<std::string, size_t> object_by_id{};
    std::unordered_map<uint32_t, size_t>    sprite_by_id{};
    std::unordered_map<std::string, size_t> sprite_by_name{};
    std::unordered_map<uint32_t, size_t>    tile_def_by_id{};
    std::unordered_map<std::string, size_t> tile_def_by_name{};
    std::unordered_map<uint32_t, size_t>    tile_map_by_id{};
    std::unordered_map<std::string, size_t> tile_map_by_name{};

    PackToc toc {};

    Pack(const std::string &path) : path(path) {}

    GfxFile &FindGraphic(const std::string &id) {
        const auto &entry = gfx_file_by_id.find(id);
        if (entry != gfx_file_by_id.end()) {
            return gfx_files[entry->second];
        } else {
            //TraceLog(LOG_WARNING, "Missing graphic file: %s", id.c_str());
            return gfx_files[0];
        }
    }

    MusFile &FindMusic(const std::string &id) {
        const auto &entry = mus_file_by_id.find(id);
        if (entry != mus_file_by_id.end()) {
            return mus_files[entry->second];
        } else {
            //TraceLog(LOG_WARNING, "Missing music: %s", id.c_str());
            return mus_files[0];
        }
    }

    SfxFile &FindSoundVariant(const std::string &id) {
        const auto &entry = sfx_file_by_id.find(id);
        if (entry != sfx_file_by_id.end()) {
            const auto &variants = entry->second;
            assert(!variants.empty());

            size_t sfx_idx;
            if (variants.size() > 1) {
                const size_t variant_idx = (size_t)GetRandomValue(0, variants.size() - 1);
                sfx_idx = variants[variant_idx];
            } else {
                sfx_idx = variants[0];
            }

            return sfx_files[sfx_idx];
        } else {
            if (id != "null") {
                TraceLog(LOG_WARNING, "Missing sound: %s", id.c_str());
            }
            return sfx_files[0];
        }
    }

    GfxFrame &FindGraphicFrame(const std::string &id) {
        const auto &entry = gfx_frame_by_id.find(id);
        if (entry != gfx_frame_by_id.end()) {
            return gfx_frames[entry->second];
        } else {
            if (id != "null") {
                TraceLog(LOG_WARNING, "Missing graphic frame: %s", id.c_str());
            }
            return gfx_frames[0];
        }
    }

    GfxAnim &FindGraphicAnim(const std::string &id) {
        const auto &entry = gfx_anim_by_id.find(id);
        if (entry != gfx_anim_by_id.end()) {
            return gfx_anims[entry->second];
        } else {
            if (id != "null") {
                TraceLog(LOG_WARNING, "Missing graphic animation: %s", id.c_str());
            }
            return gfx_anims[0];
        }
    }

    TileMat &FindTileMat(const std::string &id) {
        const auto &entry = tile_mat_by_id.find(id);
        if (entry != tile_mat_by_id.end()) {
            return tile_mats[entry->second];
        } else {
            if (id != "null") {
                TraceLog(LOG_WARNING, "Missing tile material: %s", id.c_str());
            }
            return tile_mats[0];
        }
    }

    Sprite &FindSprite(uint32_t id) {
        const auto &entry = sprite_by_id.find(id);
        if (entry != sprite_by_id.end()) {
            return sprites[entry->second];
        } else {
            if (id) {
                TraceLog(LOG_WARNING, "Missing sprite: %u", id);
            }
            return sprites[0];
        }
    }

    Sprite &FindSpriteByName(const std::string &name) {
        const auto &entry = sprite_by_name.find(name);
        if (entry != sprite_by_name.end()) {
            return sprites[entry->second];
        } else {
            if (name != "null") {
                TraceLog(LOG_WARNING, "Missing sprite by name: %s", name.c_str());
            }
            return sprites[0];
        }
    }

    TileDef &FindTileDefById(uint32_t id) {
        const auto &entry = tile_def_by_id.find(id);
        if (entry != tile_def_by_id.end()) {
            return tile_defs[entry->second];
        } else {
            TraceLog(LOG_WARNING, "Missing tile definition by id: %u", id);
            return tile_defs[0];
        }
    }

    TileDef &FindTileDefByName(const std::string &name) {
        const auto &entry = tile_def_by_name.find(name);
        if (entry != tile_def_by_name.end()) {
            return tile_defs[entry->second];
        } else {
            if (name != "null") {
                TraceLog(LOG_WARNING, "Missing tile definition by name: %s", name.c_str());
            }
            return tile_defs[0];
        }
    }

    Tilemap &FindTilemap(uint32_t id) {
        const auto &entry = tile_map_by_id.find(id);
        if (entry != tile_map_by_id.end()) {
            return tile_maps[entry->second];
        } else {
            TraceLog(LOG_WARNING, "Missing tile map: %u", id);
            return tile_maps[0];
        }
    }

    Tilemap &FindTilemapByName(const std::string &name) {
        const auto &entry = tile_map_by_name.find(name);
        if (entry != tile_map_by_name.end()) {
            return tile_maps[entry->second];
        } else {
            TraceLog(LOG_WARNING, "Missing tile map: %s", name.c_str());
            return tile_maps[0];
        }
    }

    size_t FindTilemapIndex(uint32_t id) {
        const auto &entry = tile_map_by_id.find(id);
        if (entry != tile_map_by_id.end()) {
            return entry->second;
        } else {
            TraceLog(LOG_WARNING, "Missing tile map: %u", id);
            return 0;
        }
    }
};

enum PackStreamType {
    PACK_TYPE_BINARY,
    //PACK_TYPE_TEXT,
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

extern std::vector<Pack> packs;