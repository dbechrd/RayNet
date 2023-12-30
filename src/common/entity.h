#pragma once

struct Msg_S_EntitySpawn;
struct Msg_S_EntitySnapshot;

// Best rap song: "i added it outta habit" by dandymcgee
#define ENTITY_TYPES(gen) \
gen(ENTITY_NONE)          \
gen(ENTITY_PLAYER)        \
gen(ENTITY_NPC)           \
gen(ENTITY_ITEM)          \
gen(ENTITY_PROJECTILE)

enum EntityType : uint8_t {
    ENTITY_TYPES(ENUM_V_VALUE)
};

#define ENTITY_SPECIES(gen)   \
gen(ENTITY_SPEC_NONE)         \
gen(ENTITY_SPEC_NPC_TOWNFOLK) \
gen(ENTITY_SPEC_NPC_CHICKEN)  \
gen(ENTITY_SPEC_ITM_NORMAL)   \
gen(ENTITY_SPEC_PRJ_FIREBALL)

enum EntitySpecies : uint8_t {
    ENTITY_SPECIES(ENUM_V_VALUE)
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
    uint32_t      path_id              {};
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
    uint8_t last_processed_input_cmd {};

    GhostSnapshot(void) {}
    GhostSnapshot(Msg_S_EntitySpawn &msg);
    GhostSnapshot(Msg_S_EntitySnapshot &msg);
};
typedef RingBuffer<GhostSnapshot, CL_SNAPSHOT_COUNT> AspectGhost;

// std::string -> StringId
// 672 bytes -> 296 bytes
struct Entity {
    static const DataType dtype = DAT_TYP_ENTITY;

    //// Entity ////
    uint32_t      id            {};
    EntityType    type          {};
    EntitySpecies spec          {};
    std::string   name          {};
    uint32_t      caused_by     {};  // who spawned the projectile?
    double        spawned_at    {};
    double        despawned_at  {};
    AspectGhost   *ghost        {};

    uint32_t      map_id        {};
    Vector3       position      {};

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
    uint32_t path_id                {};
    int      path_node_last_reached {};
    int      path_node_target       {};
    double   path_node_arrived_at   {};

    Vector3 path_rand_direction  {};  // move this direction (if possible)
    double  path_rand_duration   {};  // for this long
    double  path_rand_started_at {};  // when we started moving this way

    //// Physics ////
    float   drag          {};
    float   speed         {};
    Vector3 force_accum   {};
    Vector3 velocity      {};
    double  last_moved_at {};

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
    bool Attack(double now);
    void TakeDamage(int damage);
    bool Alive(void);
    bool Dead(void);
    bool Active(double now);
    void ApplyForce(Vector3 force);
};
