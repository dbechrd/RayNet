#pragma once
#include "common.h"

////////////////////////////////////////////////////////////////////////////
// TileFrame: textureId, x, y, w, h
// TileGraphic: uint8_t id, const char *name, animLen[8], frames[8], delays[8] (up to 8 anims, for each direction)
// TileMaterial: flags (walk, swim), footstepSoundId
// TileType: uint8_t id, tileGraphicId, tileMaterialId
////////////////////////////////////////////////////////////////////////////

namespace data {
    enum DataType : uint8_t {
        DAT_TYP_ARRAY,
        DAT_TYP_GFX_FILE,
        DAT_TYP_MUS_FILE,
        DAT_TYP_SFX_FILE,
        DAT_TYP_GFX_FRAME,
        DAT_TYP_GFX_ANIM,
        DAT_TYP_SPRITE,
        DAT_TYP_MATERIAL,
        DAT_TYP_TILE_TYPE,
        DAT_TYP_ENTITY,
        DAT_TYP_COUNT
    };

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

#define GFX_FILE_IDS(gen)        \
    gen(GFX_FILE_NONE)           \
    gen(GFX_FILE_DLG_NPATCH)     \
    gen(GFX_FILE_CHR_MAGE)       \
    gen(GFX_FILE_NPC_LILY)       \
    gen(GFX_FILE_OBJ_CAMPFIRE)   \
    gen(GFX_FILE_PRJ_FIREBALL)   \
    gen(GFX_FILE_TIL_OVERWORLD)  \
    gen(GFX_FILE_TIL_AUTO_GRASS)

    enum GfxFileId : uint16_t {
        GFX_FILE_IDS(ENUM_GEN_VALUE)
    };
    struct GfxFile {
        static const DataType dtype = DAT_TYP_GFX_FILE;
        GfxFileId   id      {};
        std::string path    {};
        ::Texture   texture {};
    };

#define MUS_FILE_IDS(gen)           \
    gen(MUS_FILE_NONE)              \
    gen(MUS_FILE_AMBIENT_OUTDOORS)  \
    gen(MUS_FILE_AMBIENT_CAVE)      \

    enum MusFileId : uint16_t {
        MUS_FILE_IDS(ENUM_GEN_VALUE)
    };
    struct MusFile {
        static const DataType dtype = DAT_TYP_MUS_FILE;
        MusFileId   id    {};
        std::string path  {};
        ::Music     music {};
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
        ::Sound     sound          {};
    };

#define GFX_FRAME_IDS(gen)           \
    gen(GFX_FRAME_NONE)              \
    gen(GFX_FRAME_CHR_MAGE_E_0)      \
    gen(GFX_FRAME_CHR_MAGE_W_0)      \
    gen(GFX_FRAME_NPC_LILY_E_0)      \
    gen(GFX_FRAME_NPC_LILY_W_0)      \
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
        GfxAnimId  id         {};
        SfxFileId  sound      {};
        uint8_t    frameRate  {};
        uint8_t    frameCount {};
        uint8_t    frameDelay {};
        GfxFrameId frames[8]  {};

        bool soundPlayed{};
    };

    struct Sprite {
        static const DataType dtype = DAT_TYP_SPRITE;
        Direction dir       {};
        GfxAnimId anims[8]  {};  // for each direction
        uint8_t   animFrame {};  // current frame index
        double    animAccum {};  // time since last update
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
        TileTypeId  id           {};
        GfxAnimId   anim         {};
        MaterialId  material     {};
        TileFlags   flags        {};
        uint8_t     autoTileMask {};
        //Color color;  // color for minimap/wang tile editor (top left pixel of tile)
    };

    ////////////////////////////////////////////////////////////////////////////

    // Best rap song: "i added it outta habit" by dandymcgee
#define ENTITY_TYPES(gen)  \
    gen(ENTITY_NONE)       \
    gen(ENTITY_PLAYER)     \
    gen(ENTITY_NPC)        \
    gen(ENTITY_PROJECTILE)

    enum EntityType : uint8_t {
        ENTITY_TYPES(ENUM_GEN_VALUE)
    };

    struct Entity {
        static const DataType dtype = DAT_TYP_ENTITY;
        uint32_t    id             {};
        uint32_t    mapId          {};
        EntityType  type           {};
        double      spawnedAt      {};
        double      despawnedAt    {};
        Vector2     position       {};

        // TODO: Separate this out into its own array?
        uint32_t freelist_next {};
    };

    struct AspectCollision {
        float radius    {};  // collision
        bool  colliding {};  // not sync'd, local flag for debugging colliders
        bool  onWarp    {};  // currently colliding with a warp (used to prevent ping-ponging)
    };

    struct AspectCombat {
        double lastAttackedAt {};
        double attackCooldown {};
    };

    struct AspectDialog {
        double      spawnedAt {};  // time when dialog was spawned
        std::string message   {};  // what they're saying
    };

    struct AspectLife {
        float maxHealth;
        float health;
        float healthSmooth;  // client-only to smoothly interpolate health changes

        void TakeDamage(int damage) {
            if (damage >= health) {
                health = 0;
            } else {
                health -= damage;
            }
        }

        bool Alive(void) {
            return health > 0;
        }

        bool Dead(void) {
            return !Alive();
        }
    };

    struct AspectPathfind {
        bool active;  // if false, don't pathfind
        int pathId;
        int pathNodeLastArrivedAt;
        int pathNodeTarget;
        double pathNodeArrivedAt;
    };

    struct AspectPhysics {
        float drag;
        float speed;
        Vector2 forceAccum;
        Vector2 velocity;

        void ApplyForce(Vector2 force) {
            forceAccum.x += force.x;
            forceAccum.y += force.y;
        }
    };

    struct AspectWarp {
        Rectangle collider{};
        Vector2 destPos{};

        // You either need this
        std::string destMap{};          // regular map to warp to
        // Or both of these
        std::string templateMap{};      // template map to make a copy of for procgen
        std::string templateTileset{};  // wang tileset to use for procgen
    };

    ////////////////////////////////////////////////////////////////////////////

    struct PackTocEntry {
        DataType dtype  {};
        int      offset {};

        PackTocEntry() = default;
        PackTocEntry(DataType dtype, int offset) : dtype(dtype), offset(offset) {}
    };

    struct PackToc {
        std::vector<PackTocEntry> entries;
    };

    // Inspired by https://twitter.com/angealbertini/status/1340712669247119360
    struct Pack {
        int magic     {};
        int version   {};
        int tocOffset {};

        std::vector<GfxFile>  gfxFiles  {};
        std::vector<MusFile>  musFiles  {};
        std::vector<SfxFile>  sfxFiles  {};
        std::vector<GfxFrame> gfxFrames {};
        std::vector<GfxAnim>  gfxAnims  {};
        std::vector<Material> materials {};
        std::vector<TileType> tileTypes {};

        std::vector<Entity>          entities  {SV_MAX_ENTITIES};
        std::vector<AspectCombat>    combat    {SV_MAX_ENTITIES};
        std::vector<AspectCollision> collision {SV_MAX_ENTITIES};
        std::vector<AspectDialog>    dialog    {SV_MAX_ENTITIES};
        std::vector<AspectLife>      life      {SV_MAX_ENTITIES};
        std::vector<AspectPathfind>  pathfind  {SV_MAX_ENTITIES};
        std::vector<AspectPhysics>   physics   {SV_MAX_ENTITIES};
        std::vector<Sprite>          sprite    {SV_MAX_ENTITIES};
        std::vector<AspectWarp>      warp      {SV_MAX_ENTITIES};

        PackToc toc {};
    };

    //enum PackStreamMode {
    //    PACK_MODE_READ,
    //    PACK_MODE_WRITE,
    //};

    typedef size_t (*ProcessFn)(void *buffer, size_t size, size_t count, FILE* stream);

    struct PackStream {
        //PackStreamMode mode;
        FILE *    f       {};
        ProcessFn process {};
        Pack *    pack    {};
    };

    const char *GfxFileIdStr(GfxFileId id);
    const char *MusFileIdStr(MusFileId id);
    const char *SfxFileIdStr(SfxFileId id);
    const char *GfxFrameIdStr(GfxFrameId id);
    const char *GfxAnimIdStr(GfxAnimId id);
    const char *MaterialIdStr(MaterialId id);
    const char *TileTypeIdStr(TileTypeId id);
    const char *EntityTypeStr(EntityType type);

    extern Pack pack1;

    void Init(void);
    void Free(void);

    Err Save(const char *filename);
    Err Load(const char *filename);

    void PlaySound(SfxFileId id, float pitchVariance = 0.0f);

    const GfxFrame &GetSpriteFrame(const Sprite &sprite);
    void UpdateSprite(Sprite &sprite, EntityType entityType, Vector2 velocity, double dt, bool newlySpawned);
    void ResetSprite(Sprite &sprite);
    void DrawSprite(const Sprite &sprite, Vector2 pos);
}
////////////////////////////////////////////////////////////////////////////