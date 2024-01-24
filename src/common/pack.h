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

    std::unordered_map<std::string, std::vector<uint16_t>> gfx_frame_ids_by_gfx_file_name{};

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
    bool AddToIndex(T &dat, size_t index)
    {
        auto &by_id = dat_by_id[T::dtype];
        if (by_id.find(dat.id) != by_id.end()) {
            TraceLog(LOG_ERROR, "pack already contains type %s with id %u", DataTypeStr(T::dtype), dat.id);
            return false;
        }

        auto &by_name = dat_by_name[T::dtype];
        if (dat.name.size() && by_name.find(dat.name) != by_name.end()) {
            TraceLog(LOG_ERROR, "pack already contains type %s with name '%s'", DataTypeStr(T::dtype), dat.name.c_str());
            return false;
        }

        if constexpr (dat.dtype == GfxFrame::dtype) {
            gfx_frame_ids_by_gfx_file_name[dat.gfx].push_back(dat.id);
        } else if constexpr (dat.dtype == Tilemap::dtype) {
            for (uint16_t i = 0; i < dat.object_data.size(); i++) {
                ObjectData &obj = dat.object_data[i];
                Tilemap::Coord coord{ obj.x, obj.y };
                dat.obj_by_coord[coord] = i;
            }
        }

        by_id[dat.id] = index;
        by_name[dat.name] = index;
        return true;
    }

    template <typename T>
    T *Add(T &dat)
    {
        auto &vec = *(std::vector<T> *)GetPool(T::dtype);
        uint16_t max_id = 0;
        for (T &dat : vec) max_id = MAX(max_id, dat.id);

        size_t index = vec.size();
        dat.id = max_id + 1;
        if (AddToIndex(dat, index)) {
            vec.push_back(dat);
            return &vec.back();
        }
        return 0;
    }

    template <typename T>
    T *FindByIdTry(uint16_t id) {
        auto &vec = *(std::vector<T> *)GetPool(T::dtype);
        const auto &map = dat_by_id[T::dtype];
        const auto &entry = map.find(id);
        if (entry != map.end()) {
            return &vec[entry->second];
        } else {
            return 0;
        }
    }

    template <typename T>
    T *FindByNameTry(const std::string &name) {
        const DataType dtype = T::dtype;
        auto &vec = *(std::vector<T> *)GetPool(T::dtype);
        const auto &map = dat_by_name[dtype];
        const auto &entry = map.find(name);
        if (entry != map.end()) {
            return &vec[entry->second];
        } else {
            return 0;
        }
    }

    template <typename T>
    T &FindById(uint16_t id) {
        T *val = FindByIdTry<T>(id);
        if (val) {
            return *val;
        } else {
            //TraceLog(LOG_WARNING, "Could not find resource of type %s by id '%u'.", DataTypeStr(T::dtype), id);
            auto &vec = *(std::vector<T> *)GetPool(T::dtype);
            return vec[0];
        };
    }

    template <typename T>
    T &FindByName(const std::string &name) {
        T *val = FindByNameTry<T>(name);
        if (val) {
            return *val;
        } else {
            //TraceLog(LOG_WARNING, "Could not find resource of type %s by name '%s'.", DataTypeStr(T::dtype), name.c_str());
            auto &vec = *(std::vector<T> *)GetPool(T::dtype);
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