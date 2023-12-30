#pragma once

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

enum PackStreamType {
    PACK_TYPE_BINARY,
    PACK_TYPE_TEXT
};

// Inspired by https://twitter.com/angealbertini/status/1340712669247119360
struct Pack {
    std::string name       {};
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

    Pack(const std::string &name) : name(name) {}

    std::string GetPath(PackStreamType type)
    {
        std::string path = "pack/" + name + (type == PACK_TYPE_BINARY ? ".dat" : ".txt");
        return path;
    }

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

Err SavePack(Pack &pack, PackStreamType type, std::string path = "");
Err LoadPack(Pack &pack, PackStreamType type, std::string path = "");
Err LoadResources(Pack &pack);
void UnloadPack(Pack &pack);