#pragma once
#include "common.h"

////////////////////////////////////////////////////////////////////////////
// TileFrame: textureId, x, y, w, h
// TileGraphic: uint8_t id, const char *name, animLen[8], frames[8], delays[8] (up to 8 anims, for each direction)
// TileMaterial: flags (walk, swim), footstepSoundId
// TileType: uint8_t id, tileGraphicId, tileMaterialId
////////////////////////////////////////////////////////////////////////////

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
        gen(DAT_TYP_SPRITE,    "SPRITE"   )

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

#define GFX_FILE_IDS(gen)        \
    gen(GFX_FILE_NONE)           \
    gen(GFX_FILE_DLG_NPATCH)     \
    gen(GFX_FILE_CHR_MAGE)       \
    gen(GFX_FILE_NPC_LILY)       \
    gen(GFX_FILE_NPC_FREYE)      \
    gen(GFX_FILE_NPC_NESSA)      \
    gen(GFX_FILE_NPC_ELANE)      \
    gen(GFX_FILE_OBJ_CAMPFIRE)   \
    gen(GFX_FILE_PRJ_FIREBALL)   \
    gen(GFX_FILE_TIL_OVERWORLD)  \
    gen(GFX_FILE_TIL_AUTO_GRASS)

    enum GfxFileId : uint16_t {
        GFX_FILE_IDS(ENUM_GEN_VALUE)
    };
    struct GfxFile {
        static const DataType dtype = DAT_TYP_GFX_FILE;
        GfxFileId   id          {};
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
#endif

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

    struct MusFile {
        static const DataType dtype = DAT_TYP_MUS_FILE;
        std::string id          {};
        std::string path        {};
        DatBuffer   data_buffer {};
        ::Music     music       {};
    };

#define SFX_FILE_IDS(gen)        \
    gen(SFX_FILE_NONE)           \
    gen(SFX_FILE_SOFT_TICK)      \
    gen(SFX_FILE_CAMPFIRE)       \
    gen(SFX_FILE_FOOTSTEP_GRASS) \
    gen(SFX_FILE_FOOTSTEP_STONE) \
    gen(SFX_FILE_FIREBALL)

    enum SfxFileId : uint16_t {
        SFX_FILE_IDS(ENUM_GEN_VALUE)
    };
    struct SfxFile {
        static const DataType dtype = DAT_TYP_SFX_FILE;
        SfxFileId   id             {};
        std::string path           {};
        float       pitch_variance {};
        bool        multi          {};
        DatBuffer   data_buffer    {};
        ::Sound     sound          {};
    };

#define GFX_FRAME_IDS(gen)           \
    gen(GFX_FRAME_NONE)              \
    gen(GFX_FRAME_CHR_MAGE_E_0)      \
    gen(GFX_FRAME_CHR_MAGE_W_0)      \
    gen(GFX_FRAME_NPC_LILY_E_0)      \
    gen(GFX_FRAME_NPC_LILY_W_0)      \
    gen(GFX_FRAME_NPC_FREYE_E_0)     \
    gen(GFX_FRAME_NPC_FREYE_W_0)     \
    gen(GFX_FRAME_NPC_NESSA_E_0)     \
    gen(GFX_FRAME_NPC_NESSA_W_0)     \
    gen(GFX_FRAME_NPC_ELANE_E_0)     \
    gen(GFX_FRAME_NPC_ELANE_W_0)     \
    gen(GFX_FRAME_OBJ_CAMPFIRE_0)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_1)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_2)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_3)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_4)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_5)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_6)    \
    gen(GFX_FRAME_OBJ_CAMPFIRE_7)    \
    /* gen(GFX_FRAME_OBJ_FLAG_0) */  \
    /* gen(GFX_FRAME_OBJ_FLAG_1) */  \
    /* gen(GFX_FRAME_OBJ_FLAG_2) */  \
    /* gen(GFX_FRAME_OBJ_FLAG_3) */  \
    gen(GFX_FRAME_PRJ_FIREBALL_0)    \
    gen(GFX_FRAME_TIL_GRASS)         \
    gen(GFX_FRAME_TIL_STONE_PATH)    \
    gen(GFX_FRAME_TIL_WALL)          \
    gen(GFX_FRAME_TIL_WATER_0)       \
    gen(GFX_FRAME_TIL_AUTO_GRASS_00) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_01) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_02) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_03) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_04) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_05) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_06) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_07) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_08) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_09) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_10) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_11) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_12) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_13) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_14) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_15) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_16) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_17) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_18) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_19) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_20) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_21) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_22) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_23) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_24) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_25) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_26) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_27) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_28) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_29) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_30) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_31) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_32) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_33) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_34) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_35) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_36) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_37) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_38) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_39) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_40) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_41) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_42) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_43) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_44) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_45) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_46) \
    gen(GFX_FRAME_TIL_AUTO_GRASS_47)

    enum GfxFrameId : uint16_t {
        GFX_FRAME_IDS(ENUM_GEN_VALUE)
    };
    struct GfxFrame {
        static const DataType dtype = DAT_TYP_GFX_FRAME;
        GfxFrameId id  {};
        GfxFileId  gfx {};
        uint16_t   x   {};
        uint16_t   y   {};
        uint16_t   w   {};
        uint16_t   h   {};
    };

#define GFX_ANIM_IDS(gen)            \
    gen(GFX_ANIM_NONE)               \
    gen(GFX_ANIM_CHR_MAGE_E)         \
    gen(GFX_ANIM_CHR_MAGE_W)         \
    gen(GFX_ANIM_NPC_LILY_E)         \
    gen(GFX_ANIM_NPC_LILY_W)         \
    gen(GFX_ANIM_NPC_FREYE_E)        \
    gen(GFX_ANIM_NPC_FREYE_W)        \
    gen(GFX_ANIM_NPC_NESSA_E)        \
    gen(GFX_ANIM_NPC_NESSA_W)        \
    gen(GFX_ANIM_NPC_ELANE_E)        \
    gen(GFX_ANIM_NPC_ELANE_W)        \
    gen(GFX_ANIM_OBJ_CAMPFIRE)       \
    gen(GFX_ANIM_PRJ_FIREBALL)       \
    /* gen(GFX_ANIM_FLAG) */         \
    gen(GFX_ANIM_TIL_GRASS)          \
    gen(GFX_ANIM_TIL_STONE_PATH)     \
    gen(GFX_ANIM_TIL_WALL)           \
    gen(GFX_ANIM_TIL_WATER)          \
    gen(GFX_ANIM_TIL_AUTO_GRASS_00)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_01)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_02)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_03)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_04)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_05)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_06)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_07)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_08)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_09)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_10)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_11)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_12)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_13)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_14)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_15)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_16)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_17)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_18)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_19)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_20)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_21)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_22)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_23)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_24)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_25)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_26)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_27)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_28)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_29)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_30)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_31)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_32)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_33)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_34)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_35)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_36)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_37)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_38)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_39)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_40)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_41)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_42)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_43)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_44)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_45)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_46)  \
    gen(GFX_ANIM_TIL_AUTO_GRASS_47)

    enum GfxAnimId : uint16_t {
        GFX_ANIM_IDS(ENUM_GEN_VALUE)
    };
    struct GfxAnim {
        static const DataType dtype = DAT_TYP_GFX_ANIM;
        GfxAnimId  id          {};
        SfxFileId  sound       {};
        uint8_t    frame_rate  {};
        uint8_t    frame_count {};
        uint8_t    frame_delay {};
        GfxFrameId frames[8]   {};

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
        MaterialId id          {};
        SfxFileId  footstepSnd {};
    };

#define SPRITE_IDS(gen)        \
    gen(SPRITE_NONE)           \
    gen(SPRITE_CHR_MAGE)       \
    gen(SPRITE_NPC_LILY)       \
    gen(SPRITE_NPC_FREYE)      \
    gen(SPRITE_NPC_NESSA)      \
    gen(SPRITE_NPC_ELANE)      \
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
        SpriteId  id       {};
        GfxAnimId anims[8] {};  // for each direction
    };

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
        GfxAnimId   anim           {};
        MaterialId  material       {};
        TileFlags   flags          {};
        uint8_t     auto_tile_mask {};
        //Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    };

    ////////////////////////////////////////////////////////////////////////////

    // Best rap song: "i added it outta habit" by dandymcgee
#define ENTITY_TYPES(gen)  \
    gen(ENTITY_NONE)       \
    gen(ENTITY_PLAYER)     \
    gen(ENTITY_NPC)        \
    gen(ENTITY_PROJECTILE) \
    gen(ENTITY_WARP)

    enum EntityType : uint8_t {
        ENTITY_TYPES(ENUM_GEN_VALUE)
    };

    struct EntityProto {
        EntityType  type            {};
        float       radius          {};
        std::string dialog_root_key {};
        float       hp_max          {};
        int         path_id         {};  // if non-zero, set path_active = true
        float       speed_min       {};
        float       speed_max       {};
        float       drag            {};
        SpriteId    sprite          {};
    };

    struct Entity {
        static const DataType dtype = DAT_TYP_ENTITY;

        //// Entity ////
        uint32_t   id           {};
        uint32_t   map_id       {};
        EntityType type         {};
        uint32_t   caused_by    {};
        double     spawned_at   {};
        double     despawned_at {};
        Vector2    position     {};

        // TODO: Separate this out into its own array?
        uint32_t freelist_next {};

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

        //// Physics ////
        float   drag        {};
        float   speed       {};
        Vector2 force_accum {};
        Vector2 velocity    {};

        void ApplyForce(Vector2 force) {
            force_accum.x += force.x;
            force_accum.y += force.y;
        }

        //// Sprite ////
        SpriteId  sprite     {};  // sprite resource
        Direction direction  {};  // current facing direction
        uint8_t   anim_frame {};  // current frame index
        double    anim_accum {};  // time since last update

        //// Warp ////
        Rectangle warp_collider {};
        Vector2   warp_dest_pos {};

        // You either need this
        std::string warp_dest_map         {};  // regular map to warp to
        // Or both of these
        std::string warp_template_map     {};  // template map to make a copy of for procgen
        std::string warp_template_tileset {};  // wang tileset to use for procgen
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
        std::vector<GfxFile>  gfx_files  {};
        std::vector<MusFile>  mus_files  {};
        std::vector<SfxFile>  sfx_files  {};
        std::vector<GfxFrame> gfx_frames {};
        std::vector<GfxAnim>  gfx_anims  {};
        std::vector<Material> materials  {};
        std::vector<Sprite>   sprites    {};
        std::vector<TileType> tile_types {};

        std::unordered_map<std::string, size_t> musFilesById{};

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
        std::vector<Entity> entities {SV_MAX_ENTITIES};

        PackToc toc {};

        Pack(std::string path) : path(path) {}

        MusFile &FindMusic(std::string id) {
            const auto &entry = musFilesById.find(id);
            if (entry == musFilesById.end()) {
                return mus_files[0];
            } else {
                return mus_files[entry->second];
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
    const char *GfxFileIdStr(GfxFileId id);
    //const char *MusFileIdStr(MusFileId id);
    const char *SfxFileIdStr(SfxFileId id);
    const char *GfxFrameIdStr(GfxFrameId id);
    const char *GfxAnimIdStr(GfxAnimId id);
    const char *MaterialIdStr(MaterialId id);
    const char *TileTypeIdStr(TileTypeId id);
    const char *EntityTypeStr(EntityType type);

    extern Pack pack1;
    extern Pack *packs[1];

    void ReadFileIntoDataBuffer(std::string filename, DatBuffer &datBuffer);
    void FreeDataBuffer(DatBuffer &datBuffer);

    void Init(void);
    void Free(void);

    Err SavePack(Pack &pack);
    Err LoadPack(Pack &pack);
    void UnloadPack(Pack &pack);

    void PlaySound(SfxFileId id, float pitchVariance = 0.0f);
    bool IsSoundPlaying(SfxFileId id);
    void StopSound(SfxFileId id);

    const GfxFrame &GetSpriteFrame(const Entity &entity);
    void UpdateSprite(Entity &entity, double dt, bool newlySpawned);
    void ResetSprite(Entity &entity);
    void DrawSprite(const Entity &entity, Vector2 pos);
}
////////////////////////////////////////////////////////////////////////////