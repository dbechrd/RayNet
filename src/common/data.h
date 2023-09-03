#pragma once
#include "common.h"

////////////////////////////////////////////////////////////////////////////
// TileFrame: textureId, x, y, w, h
// TileGraphic: uint8_t id, const char *name, animLen[8], frames[8], delays[8] (up to 8 anims, for each direction)
// TileMaterial: flags (walk, swim), footstepSoundId
// TileType: uint8_t id, tileGraphicId, tileMaterialId
////////////////////////////////////////////////////////////////////////////

struct Msg_S_TileChunk;
struct Msg_S_EntitySnapshot;
struct WangMap;

namespace data {
    // DO NOT re-order these! They are hard-coded in the TOC
#define DATA_TYPES(gen) \
        gen(DAT_TYP_UNUSED,    "UNUSED"   ) \
        gen(DAT_TYP_GFX_FILE,  "GFX_FILE" ) \
        gen(DAT_TYP_MUS_FILE,  "MUS_FILE" ) \
        gen(DAT_TYP_SFX_FILE,  "SFX_FILE" ) \
        gen(DAT_TYP_GFX_FRAME, "GFX_FRAME") \
        gen(DAT_TYP_GFX_ANIM,  "GFX_ANIM" ) \
        gen(DAT_TYP_MATERIAL,  "MATERIAL" ) \
        gen(DAT_TYP_TILE_TYPE, "TILE_TYPE") \
        gen(DAT_TYP_ENTITY,    "ENTITY"   ) \
        gen(DAT_TYP_SPRITE,    "SPRITE"   ) \
        gen(DAT_TYP_TILE_MAP,  "TILE_MAP" )

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

//#define MUS_FILE_IDS(gen)           \
//    gen(MUS_FILE_NONE)              \
//    gen(MUS_FILE_AMBIENT_OUTDOORS)  \
//    gen(MUS_FILE_AMBIENT_CAVE)      \
//
//    enum MusFileId : uint16_t {
//        MUS_FILE_IDS(ENUM_GEN_VALUE)
//    };

#if 0
    struct Idx {
        uint16_t index      {};
        uint16_t generation {};
    };

    // Idea: Store all resources in a giant array. When looking up, if the index
    // is valid check and generation matches, return resource. Otherwise, look
    // up the resource by name and populate index/generation.
    struct ResId {
        const char *id         {};
        uint16_t    index      {};
        uint16_t    generation {};
    };

    struct MusFile_Dat {
        static const DataType dtype = DAT_TYP_MUS_FILE;
        DatBuffer buf {};
    };

    struct MusFile_Mem {
        MusFile_Dat * dat   {};
        ::Music       music {};
    };

    struct MapArea_Dat {
        const char *musAmbientId;
    };

    struct MapArea_Mem {

    };
#endif

    struct MusFile {
        static const DataType dtype = DAT_TYP_MUS_FILE;
        std::string id          {};
        std::string path        {};
        DatBuffer   data_buffer {};
        ::Music     music       {};

        MusFile(void) = default;
        MusFile(std::string id, std::string path) : id(id), path(path) {}
    };

    struct SfxFile {
        static const DataType dtype = DAT_TYP_SFX_FILE;
        std::string id             {};
        std::string path           {};
        int         variations     {};  // how many version of the file to look for with "_00" suffix
        float       pitch_variance {};
        bool        multi          {};
        DatBuffer   data_buffer    {};
        ::Sound     sound          {};

        SfxFile(void) = default;
        SfxFile(std::string id, std::string path, float pitch_variance, bool multi)
            : id(id), path(path), pitch_variance(pitch_variance), multi(multi) {}
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
    };

#define MATERIAL_IDS(gen) \
    gen(MATERIAL_NONE)    \
    gen(MATERIAL_GRASS)   \
    gen(MATERIAL_STONE)   \
    gen(MATERIAL_WATER)

    enum MaterialId : uint16_t {
        MATERIAL_IDS(ENUM_GEN_VALUE)
    };

    struct Material {
        static const DataType dtype = DAT_TYP_MATERIAL;
        MaterialId  id             {};
        std::string footstep_sound {};
    };

#define SPRITE_IDS(gen)        \
    gen(SPRITE_NONE)           \
    gen(SPRITE_CHR_MAGE)       \
    gen(SPRITE_NPC_LILY)       \
    gen(SPRITE_NPC_FREYE)      \
    gen(SPRITE_NPC_NESSA)      \
    gen(SPRITE_NPC_ELANE)      \
    gen(SPRITE_NPC_CHICKEN)    \
    gen(SPRITE_OBJ_CAMPFIRE)   \
    gen(SPRITE_PRJ_FIREBALL)

    enum SpriteId : uint16_t {
        SPRITE_IDS(ENUM_GEN_VALUE)
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
        SpriteId    id       {};
        std::string anims[8] {};  // for each direction
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
    gen(ENTITY_SPEC_OBJ_WARP)

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
        SpriteId      sprite               {};
    };

    struct Entity {
        static const DataType dtype = DAT_TYP_ENTITY;

        //// Entity ////
        uint32_t      id           {};
        EntityType    type         {};
        EntitySpecies spec         {};
        std::string   name         {};
        uint32_t      caused_by    {};
        double        spawned_at   {};
        double        despawned_at {};

        uint32_t      map_id       {};
        Vector3       position     {};

        Vector2 ScreenPos(void) {
            Vector2 screenPos{ position.x, position.y - position.z };
            return screenPos;
        }

        // TODO: Separate this out into its own array?
        uint32_t freelist_next {};

        //// Audio ////
        std::string ambient_fx           {};  // some sound they play occasionally
        double      ambient_fx_delay_min {};
        double      ambient_fx_delay_max {};

        //// Collision ////
        float radius    {};  // collision
        bool  colliding {};  // not sync'd, local flag for debugging colliders
        bool  on_warp   {};  // currently colliding with a warp (used to prevent ping-ponging)

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

        void ClearDialog(void) {
            dialog_spawned_at = 0;
            dialog_title = {};
            dialog_message = {};
        }

        //// Life ////
        float hp_max    {};
        float hp        {};
        float hp_smooth {};  // client-only to smoothly interpolate health changes

        void TakeDamage(int damage) {
            if (damage >= hp) {
                hp = 0;
            } else {
                hp -= damage;
            }
        }

        bool Alive(void) {
            return hp > 0;
        }

        bool Dead(void) {
            return !Alive();
        }

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

        void ApplyForce(Vector3 force) {
            force_accum = Vector3Add(force_accum, force);
        }

        //// Sprite ////
        SpriteId  sprite     {};  // sprite resource
        Direction direction  {};  // current facing direction
        uint8_t   anim_frame {};  // current frame index
        double    anim_accum {};  // time since last update

        //// Warp ////
        Rectangle warp_collider {};
        Vector3   warp_dest_pos {};

        // You either need this
        std::string warp_dest_map         {};  // regular map to warp to
        // Or both of these
        std::string warp_template_map     {};  // template map to make a copy of for procgen
        std::string warp_template_tileset {};  // wang tileset to use for procgen
    };

    struct GhostSnapshot {
        double   server_time {};

        // Entity
        uint32_t map_id      {};
        Vector3  position    {};

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

    struct EntityData {
        data::Entity entity {};
        AspectGhost  ghosts {};
    };

    ////////////////////////////////////////////////////////////////////////////

#define TILE_TYPE_IDS(gen)  \
    gen(TILE_NONE)          \
    gen(TILE_GRASS)         \
    gen(TILE_STONE_PATH)    \
    gen(TILE_WATER)         \
    gen(TILE_WALL)          \
    gen(TILE_AUTO_GRASS_00) \
    gen(TILE_AUTO_GRASS_01) \
    gen(TILE_AUTO_GRASS_02) \
    gen(TILE_AUTO_GRASS_03) \
    gen(TILE_AUTO_GRASS_04) \
    gen(TILE_AUTO_GRASS_05) \
    gen(TILE_AUTO_GRASS_06) \
    gen(TILE_AUTO_GRASS_07) \
    gen(TILE_AUTO_GRASS_08) \
    gen(TILE_AUTO_GRASS_09) \
    gen(TILE_AUTO_GRASS_10) \
    gen(TILE_AUTO_GRASS_11) \
    gen(TILE_AUTO_GRASS_12) \
    gen(TILE_AUTO_GRASS_13) \
    gen(TILE_AUTO_GRASS_14) \
    gen(TILE_AUTO_GRASS_15) \
    gen(TILE_AUTO_GRASS_16) \
    gen(TILE_AUTO_GRASS_17) \
    gen(TILE_AUTO_GRASS_18) \
    gen(TILE_AUTO_GRASS_19) \
    gen(TILE_AUTO_GRASS_20) \
    gen(TILE_AUTO_GRASS_21) \
    gen(TILE_AUTO_GRASS_22) \
    gen(TILE_AUTO_GRASS_23) \
    gen(TILE_AUTO_GRASS_24) \
    gen(TILE_AUTO_GRASS_25) \
    gen(TILE_AUTO_GRASS_26) \
    gen(TILE_AUTO_GRASS_27) \
    gen(TILE_AUTO_GRASS_28) \
    gen(TILE_AUTO_GRASS_29) \
    gen(TILE_AUTO_GRASS_30) \
    gen(TILE_AUTO_GRASS_31) \
    gen(TILE_AUTO_GRASS_32) \
    gen(TILE_AUTO_GRASS_33) \
    gen(TILE_AUTO_GRASS_34) \
    gen(TILE_AUTO_GRASS_35) \
    gen(TILE_AUTO_GRASS_36) \
    gen(TILE_AUTO_GRASS_37) \
    gen(TILE_AUTO_GRASS_38) \
    gen(TILE_AUTO_GRASS_39) \
    gen(TILE_AUTO_GRASS_40) \
    gen(TILE_AUTO_GRASS_41) \
    gen(TILE_AUTO_GRASS_42) \
    gen(TILE_AUTO_GRASS_43) \
    gen(TILE_AUTO_GRASS_44) \
    gen(TILE_AUTO_GRASS_45) \
    gen(TILE_AUTO_GRASS_46) \
    gen(TILE_AUTO_GRASS_47)

    enum TileTypeId : uint16_t {
        TILE_TYPE_IDS(ENUM_GEN_VALUE)
    };

    typedef uint32_t TileFlags;
    enum {
        TILE_FLAG_COLLIDE = 0x01,
        TILE_FLAG_WALK    = 0x02,
        TILE_FLAG_SWIM    = 0x04,
    };

    struct TileType {
        static const DataType dtype = DAT_TYP_TILE_TYPE;
        TileTypeId  id             {};
        std::string anim           {};
        MaterialId  material       {};
        TileFlags   flags          {};
        uint8_t     auto_tile_mask {};
        //Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    };

    struct AiPathNode {
        Vector3 pos;
        double waitFor;
    };

    struct AiPath {
        uint32_t pathNodeIndexOffset;
        uint32_t pathNodeIndexCount;
    };

    struct TileDef {
        //uint16_t tileId;
        //uint16_t sheetId;
        //uint16_t materialId;
        uint16_t x;
        uint16_t y;
        uint8_t w;
        uint8_t h;
        uint8_t collide;
        uint8_t auto_tile_mask;

        //------------------------
        // Not serialized
        Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    };

    struct TileMapData {
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

        uint32_t    version   {};  // version on disk
        uint32_t    width     {};  // width of map in tiles
        uint32_t    height    {};  // height of map in tiles
        std::string name      {};  // name of map area
        std::string texture   {};  // GfxFile key

        // TODO(dlb): Move these to a global pool, each has its own textureId
        std::vector<TileDef>    tileDefs        {};
        std::vector<Tile>       tiles           {};
        std::vector<AiPathNode> pathNodes       {};  // 94 19 56 22 57
        std::vector<uint32_t>   pathNodeIndices {};  // 0 1 2 | 3 4 5
        std::vector<AiPath>     paths           {};  // offset, length | 0, 3 | 3, 3

        //-------------------------------
        // Not serialized
        //-------------------------------
        uint32_t id                 {};  // for communicating efficiently w/ client about which map
        double   chunkLastUpdatedAt {};  // used by server to know when chunks are dirty on clients

        //-------------------------------
        // Clean this section up
        //-------------------------------
        struct Coord {
            uint32_t x, y;
        };

        void SV_SerializeChunk(Msg_S_TileChunk &tileChunk, uint32_t x, uint32_t y);
        void CL_DeserializeChunk(Msg_S_TileChunk &tileChunk);

#if 1
        Err Save(std::string path);
        Err Load(std::string path);
#endif

        // Tiles
        Tile At(uint32_t x, uint32_t y);
        bool AtTry(uint32_t x, uint32_t y, Tile &tile);
        bool WorldToTileIndex(uint32_t world_x, uint32_t world_y, Coord &coord);
        bool AtWorld(uint32_t world_x, uint32_t world_y, Tile &tile);

        void Set(uint32_t x, uint32_t y, Tile tile, double now);
        void SetFromWangMap(WangMap &wangMap, double now);
        void Fill(uint32_t x, uint32_t y, int tileDefId, double now);

        const TileDef &GetTileDef(Tile tile);
        Rectangle TileDefRect(Tile tile);
        Color TileDefAvgColor(Tile tile);

        AiPath *GetPath(uint32_t pathId);
        uint32_t GetNextPathNodeIndex(uint32_t pathId, uint32_t pathNodeIndex);
        AiPathNode *GetPathNode(uint32_t pathId, uint32_t pathNodeIndex);

        void ResolveEntityTerrainCollisions(Entity &entity);
        void ResolveEntityTerrainCollisions(uint32_t entityId);

        void DrawTile(Texture2D tex, Tile tile, Vector2 position);
        void Draw(Camera2D &camera);
        void DrawColliders(Camera2D &camera);
        void DrawTileIds(Camera2D &camera);

    private:
        bool NeedsFill(uint32_t x, uint32_t y, int tileDefFill);
        void Scan(uint32_t lx, uint32_t rx, uint32_t y, Tile tileDefFill, std::stack<Coord> &stack);
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
        // - object defs
        std::vector<GfxFile>     gfx_files  {};
        std::vector<MusFile>     mus_files  {};
        std::vector<SfxFile>     sfx_files  {};
        std::vector<GfxFrame>    gfx_frames {};
        std::vector<GfxAnim>     gfx_anims  {};
        std::vector<Material>    materials  {};
        std::vector<Sprite>      sprites    {};
        std::vector<TileType>    tile_types {};
        std::vector<TileMapData> tile_maps  {};

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
        std::vector<Entity> entities {};

        std::unordered_map<std::string, size_t> gfx_file_by_id{};
        std::unordered_map<std::string, size_t> gfx_frame_by_id{};
        std::unordered_map<std::string, size_t> gfx_anim_by_id{};
        std::unordered_map<std::string, size_t> mus_file_by_id{};
        std::unordered_map<std::string, std::vector<size_t>> sfx_file_by_id{};  // vector holds variants
        std::unordered_map<std::string, size_t> tile_map_by_name{};

        PackToc toc {};

        Pack(std::string path) : path(path) {
            // Reserve slot 0 in all resource arrays for a placeholder asset
            gfx_files.emplace_back();
            mus_files.emplace_back();
            sfx_files.emplace_back();
            gfx_frames.emplace_back();
            gfx_anims.emplace_back();
            materials.emplace_back();
            sprites.emplace_back();
            tile_types.emplace_back();
            tile_maps.emplace_back();
        }

        GfxFile &FindGraphic(std::string id) {
            const auto &entry = gfx_file_by_id.find(id);
            if (entry != gfx_file_by_id.end()) {
                return gfx_files[entry->second];
            } else {
                TraceLog(LOG_WARNING, "Missing graphic file: %s", id.c_str());
                return gfx_files[0];
            }
        }

        GfxFrame &FindGraphicFrame(std::string id) {
            const auto &entry = gfx_frame_by_id.find(id);
            if (entry != gfx_frame_by_id.end()) {
                return gfx_frames[entry->second];
            } else {
                TraceLog(LOG_WARNING, "Missing graphic frame: %s", id.c_str());
                return gfx_frames[0];
            }
        }

        GfxAnim &FindGraphicAnim(std::string id) {
            const auto &entry = gfx_anim_by_id.find(id);
            if (entry != gfx_anim_by_id.end()) {
                return gfx_anims[entry->second];
            } else {
                TraceLog(LOG_WARNING, "Missing graphic animation: %s", id.c_str());
                return gfx_anims[0];
            }
        }

        MusFile &FindMusic(std::string id) {
            const auto &entry = mus_file_by_id.find(id);
            if (entry != mus_file_by_id.end()) {
                return mus_files[entry->second];
            } else {
                TraceLog(LOG_WARNING, "Missing music: %s", id.c_str());
                return mus_files[0];
            }
        }

        SfxFile &FindSound(std::string id) {
            const auto &entry = sfx_file_by_id.find(id);
            if (entry != sfx_file_by_id.end()) {
                const auto &variants = entry->second;
                assert(!variants.empty());

                size_t sfx_idx;
                if (variants.size() > 1) {
                    const size_t variant_idx = GetRandomValue(0, variants.size() - 1);
                    sfx_idx = variants[variant_idx];
                } else {
                    sfx_idx = variants[0];
                }

                return sfx_files[sfx_idx];
            } else {
                TraceLog(LOG_WARNING, "Missing sound: %s", id.c_str());
                return sfx_files[0];
            }
        }

        TileMapData &FindTileMap(std::string name) {
            const auto &entry = tile_map_by_name.find(name);
            if (entry != tile_map_by_name.end()) {
                return tile_maps[entry->second];
            } else {
                TraceLog(LOG_WARNING, "Missing tile map: %s", name.c_str());
                return tile_maps[0];
            }
        }
    };

    enum PackStreamMode {
        PACK_MODE_READ,
        PACK_MODE_WRITE,
    };

    typedef size_t (*ProcessFn)(void *buffer, size_t size, size_t count, FILE* stream);

    struct PackStream {
        PackStreamMode mode    {};
        FILE *         f       {};
        ProcessFn      process {};
        Pack *         pack    {};
    };

    const char *DataTypeStr(DataType type);
    const char *MaterialIdStr(MaterialId id);
    const char *TileTypeIdStr(TileTypeId id);
    const char *EntityTypeStr(EntityType type);

    extern std::vector<Pack *> packs;

    void ReadFileIntoDataBuffer(std::string filename, DatBuffer &datBuffer);
    void FreeDataBuffer(DatBuffer &datBuffer);

    void Init(void);
    void Free(void);

    Err SavePack(Pack &pack);
    Err LoadPack(Pack &pack);
    void UnloadPack(Pack &pack);

    void PlaySound(std::string id, float pitchVariance = 0.0f);
    bool IsSoundPlaying(std::string id);
    void StopSound(std::string id);

    const GfxFrame &GetSpriteFrame(const Entity &entity);
    Rectangle GetSpriteRect(const Entity &entity);
    void UpdateSprite(Entity &entity, double dt, bool newlySpawned);
    void ResetSprite(Entity &entity);
    void DrawSprite(const Entity &entity);
}
////////////////////////////////////////////////////////////////////////////