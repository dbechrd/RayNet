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
    ENTITY_TYPES(ENUM_GEN_VALUE)
};

#define ENTITY_SPECIES(gen)   \
gen(ENTITY_SPEC_NONE)         \
gen(ENTITY_SPEC_NPC_TOWNFOLK) \
gen(ENTITY_SPEC_NPC_CHICKEN)  \
gen(ENTITY_SPEC_ITM_NORMAL)   \
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
    uint32_t last_processed_input_cmd {};

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

namespace NewEnt {
    enum CompType {
        COMP_NAME,
        COMP_POSITION,
        COMP_LIFE,
        COMP_COUNT
    };

    struct Comp_Name {
        static const CompType type = COMP_NAME;
        std::string name {};
    };

    struct Comp_Position {
        static const CompType type = COMP_POSITION;
        uint32_t map_id   {};
        Vector3  position {};
    };

    struct Comp_Life {
        static const CompType type = COMP_LIFE;
        float hp_max    {};
        float hp        {};
    };

    struct EntityHnd {
        uint32_t idx    {};
        uint32_t gen    {};
    };

    enum EntityType : uint8_t {
        ENTITY_NONE,
        ENTITY_PLAYER,
        ENTITY_NPC,
        ENTITY_ITEM,
        ENTITY_PROJECTILE,
    };

    struct World;
    struct Entity {
        EntityHnd  hnd           {};
        uint32_t   id            {};
        EntityType type          {};
        double     spawned_at    {};
        double     despawned_at  {};

        // Components
        std::bitset<COMP_COUNT> comp_mask{};
        uint32_t comp_idx[COMP_COUNT];

        template <typename T>
        bool Add(T &dat);

        template <typename T>
        T *Get(void);
    };

    struct World {
        template <typename T>
        struct TblEntry {
            bool used {};
            T    data {};
        };

        std::vector<Entity> tbl_entity {};
        uint32_t nextId = 1;

        void *tables[COMP_COUNT] {};
        std::vector<TblEntry<Comp_Name>>     tbl_name     {};
        std::vector<TblEntry<Comp_Position>> tbl_position {};
        std::vector<TblEntry<Comp_Life>>     tbl_life     {};

        World(void) {
            tables[COMP_NAME    ] = &tbl_name;
            tables[COMP_POSITION] = &tbl_position;
            tables[COMP_LIFE    ] = &tbl_life;
        }

        Entity *AddEntity(EntityType type)
        {
            for (int i = 0; i < tbl_entity.size(); i++) {
                Entity &e = tbl_entity[i];
                if (!e.id) {
                    e.id = nextId++;  // todo check for wraparound
                    e.type = type;
                    return &e;
                }
            }

            Entity e{};
            e.hnd.idx = tbl_entity.size();
            e.id = nextId++;
            e.type = type;
            tbl_entity.push_back(e);
            return &tbl_entity.back();
        }

        Entity *GetEntity(EntityHnd hnd)
        {
            Entity &e = tbl_entity[hnd.idx];
            if (e.hnd.gen == hnd.gen) {
                return &e;
            }
            return 0;
        }

        Entity *GetEntity(uint32_t id)
        {
            assert("!finding entity by id not yet implemented, need map");
            return 0;
        }

        void RemoveEntity(EntityHnd hnd)
        {
            Entity &e = tbl_entity[hnd.idx];
            if (e.hnd.gen == hnd.gen) {
                for (int compType = 0; compType < COMP_COUNT; compType++) {
                    if (e.comp_mask.test(compType)) {
                        RemoveTableEntry((CompType)compType, e.comp_idx[compType]);
                    }
                }

                EntityHnd e_hnd = e.hnd;
                e = {};
                e.hnd = e_hnd;
                e.hnd.gen++;
            }
        }

        template <typename T>
        std::vector<TblEntry<T>> &GetTable(void)
        {
            std::vector<TblEntry<T>> &table = *(std::vector<TblEntry<T>> *)tables[T::type];
            return table;
        }

        template <typename T>
        void AddTableEntry(T &dat, uint32_t &index)
        {
            std::vector<TblEntry<T>> &table = GetTable<T>();
            for (int i = 0; i < table.size(); i++) {
                TblEntry<T> &entry = table[i];
                if (!entry.used) {
                    entry.data = dat;
                    index = i;
                    return;
                }
            }

            // no slot found, append to end
            TblEntry<T> entry{};
            entry.used = true;
            entry.data = dat;
            table.push_back(entry);
        }

        template <typename T>
        T &GetTableEntry(uint32_t index)
        {
            std::vector<TblEntry<T>> &table = GetTable<T>();
            return table[index].data;
        }

        template <typename T>
        void RemoveTableEntry(uint32_t index)
        {
            std::vector<TblEntry<T>> &table = GetTable<T>();
            TblEntry<T> &entry = table[index];
            entry.used = false;
            entry.data = {};
        }

        void RemoveTableEntry(CompType type, uint32_t index)
        {
            switch (type) {
                case COMP_NAME:     RemoveTableEntry<Comp_Name    >(index); break;
                case COMP_POSITION: RemoveTableEntry<Comp_Position>(index); break;
                case COMP_LIFE:     RemoveTableEntry<Comp_Life    >(index); break;
            }
        }
    };

    World *g_world;

    template <typename T>
    bool Entity::Add(T &entryDat) {
        if (!comp_mask.test(T::type)) {
            uint32_t &index = comp_idx[T::type];
            g_world->AddTableEntry<T>(entryDat, index);
            comp_mask.set(T::type);
            return true;
        }
        return false;
    }

    template <typename T>
    T *Entity::Get(void) {
        if (comp_mask.test(T::type)) {
            const uint32_t index = comp_idx[T::type];
            T* entryDat = &g_world->GetTableEntry<T>(index);
            return entryDat;
        }
        return 0;
    }

    struct Sys_Life {
        void TakeDamage(float dmg) {
            return;
        }
    };

    void run_tests(void)
    {
        World world{};
        g_world = &world;

        for (int i = 0; i < 2; i++) {
            Entity *e = world.AddEntity(ENTITY_PLAYER);
            assert(e);

            Vector3 spawn{ 1, 2, 3 };

            Comp_Name name{};
            name.name = "foo";
            e->Add<Comp_Name>(name);

            Comp_Position pos{};
            pos.map_id = 52;
            pos.position = spawn;
            e->Add<Comp_Position>(pos);

            Comp_Life life{};
            life.hp_max = 100;
            life.hp = life.hp_max;
            e->Add<Comp_Life>(life);

            Comp_Name     *e_name     = e->Get<Comp_Name    >();
            Comp_Position *e_position = e->Get<Comp_Position>();
            Comp_Life     *e_life     = e->Get<Comp_Life    >();

            assert(e_name->name == name.name);

            assert(e_position->map_id == pos.map_id);
            assert(e_position->position.x == pos.position.x);
            assert(e_position->position.y == pos.position.y);
            assert(e_position->position.z == pos.position.z);

            assert(e_life->hp_max == life.hp_max);
            assert(e_life->hp == life.hp);

            world.RemoveEntity(e->hnd);
        }
    }
}