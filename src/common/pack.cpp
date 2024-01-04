#include "pack.h"

#define PROC(v) Process(stream, v)

#define HAQ_IO_FIELD(c_type, c_name, c_init, flags, condition, userdata) \
    if constexpr ((flags) & HAQ_SERIALIZE) { \
        if (condition) { \
            PROC(userdata.c_name); \
        } \
    }


#define HAQ_IO(hqt, userdata) \
    hqt(HAQ_IO_FIELD, userdata);

char PeekTxt(FILE *f)
{
    char next = fgetc(f);
    fseek(f, ftell(f) - 1, SEEK_SET);
    return next;
}

void ReportTxtError(FILE *f, const char *msg)
{
    int tell = ftell(f);
    fseek(f, 0, SEEK_SET);
    int line = 1;
    int column = 1;
    for (int i = 0; i < tell; i++) {
        char c = fgetc(f);
        column++;
        if (c == '\n') {
            line++;
            column = 1;
        }
    }

    char context[40]{};
    int context_len = MIN(column - 1, sizeof(context) - 1);
    fseek(f, -context_len, SEEK_CUR);
    fgets(context, sizeof(context) - 1, f);
    fscanf(f, "%39[^\n]", context);

    printf("ERROR:%d:%d: %s\n", line, column, msg);
    printf("%s\n", context);
    printf("%*c\n", context_len, '^');

    assert(!"nope");
    exit(-1);
}

template<class T>
void Process(PackStream &stream, T &v)
{
    static_assert(std::is_fundamental_v<T> || std::is_enum_v<T>,
        "You cannot fread/write a non-primitive type");

    if (stream.type == PACK_TYPE_BINARY) {
        stream.process(&v, sizeof(v), 1, stream.file);
    } else if (stream.type == PACK_TYPE_TEXT) {
        if (stream.mode == PACK_MODE_WRITE) {
            if      constexpr (std::is_same_v<T, char    >) fprintf(stream.file, " %c", v);
            else if constexpr (std::is_same_v<T, int8_t  >) fprintf(stream.file, " %" PRId8 , v);
            else if constexpr (std::is_same_v<T, int16_t >) fprintf(stream.file, " %" PRId16, v);
            else if constexpr (std::is_same_v<T, int32_t >) fprintf(stream.file, " %" PRId32, v);
            else if constexpr (std::is_same_v<T, int64_t >) fprintf(stream.file, " %" PRId64, v);
            else if constexpr (std::is_same_v<T, uint8_t >) fprintf(stream.file, " %" PRIu8 , v);
            else if constexpr (std::is_same_v<T, uint16_t>) fprintf(stream.file, " %" PRIu16, v);
            else if constexpr (std::is_same_v<T, uint32_t>) fprintf(stream.file, " %" PRIu32, v);
            else if constexpr (std::is_same_v<T, uint64_t>) fprintf(stream.file, " %" PRIu64, v);
            else if constexpr (std::is_same_v<T, float   >) fprintf(stream.file, " %.2f", v);
            else if constexpr (std::is_same_v<T, double  >) fprintf(stream.file, " %.2f", v);
            else if constexpr (std::is_enum_v<T>)           fprintf(stream.file, " %d", v);
            else assert(!"unhandled type"); //fprintf(stream.file, " <%s>\n", typeid(T).name());
        } else {
            if      constexpr (std::is_same_v<T, char    >) fscanf(stream.file, " %c", &v);
            else if constexpr (std::is_same_v<T, int8_t  >) fscanf(stream.file, " %" PRId8 , &v);
            else if constexpr (std::is_same_v<T, int16_t >) fscanf(stream.file, " %" PRId16, &v);
            else if constexpr (std::is_same_v<T, int32_t >) fscanf(stream.file, " %" PRId32, &v);
            else if constexpr (std::is_same_v<T, int64_t >) fscanf(stream.file, " %" PRId64, &v);
            else if constexpr (std::is_same_v<T, uint8_t >) fscanf(stream.file, " %" PRIu8 , &v);
            else if constexpr (std::is_same_v<T, uint16_t>) fscanf(stream.file, " %" PRIu16, &v);
            else if constexpr (std::is_same_v<T, uint32_t>) fscanf(stream.file, " %" PRIu32, &v);
            else if constexpr (std::is_same_v<T, uint64_t>) fscanf(stream.file, " %" PRIu64, &v);
            else if constexpr (std::is_same_v<T, float   >) fscanf(stream.file, " %f", &v);
            else if constexpr (std::is_same_v<T, double  >) fscanf(stream.file, " %lf", &v);
            else if constexpr (std::is_enum_v<T>) {
                int val = -1;
                fscanf(stream.file, " %d", &val);
                v = (T)val;  // note(dlb): might be out of range, but idk how to detect that. C++ sucks.
            }
            else assert(!"unhandled type"); //fprintf(stream.file, " <%s>\n", typeid(T).name());
        }

#if _DEBUG
        if (stream.mode == PACK_MODE_WRITE) {
            fflush(stream.file);
        }
#endif
    }
}
void Process(PackStream &stream, std::string &str)
{
    uint16_t strLen = (uint16_t)str.size();
    if (stream.type == PACK_TYPE_TEXT) {
        if (stream.mode == PACK_MODE_WRITE) {
            fprintf(stream.file, " \"");
            int strLen = str.size();
            for (int i = 0; i < str.size(); i++) {
                switch (str[i]) {
                    case '"': {
                        // Escape double quotes (\")
                        fputc('\\', stream.file);
                        fputc('"', stream.file);
                        break;
                    }
                    case '\\': {
                        // Escape backslashes (\\)
                        fputc('\\', stream.file);
                        fputc('\\', stream.file);
                        break;
                    }
                    case '\n': {
                        // Escape newlines (\n)
                        fputc('\\', stream.file);
                        fputc('n', stream.file);
                        break;
                    }
                    default: {
                        const char c = str[i];
                        assert(isprint(c));
                        fputc(str[i], stream.file);
                        break;
                    }
                }
            }
            fprintf(stream.file, "\"");
        } else {
            assert(str.size() == 0);

            fscanf(stream.file, " ");

            char start_quote = fgetc(stream.file);
            if (start_quote != '"') {
                ReportTxtError(stream.file, "expected string to start with double quote");
            }

            do {
                char buf[1024]{};
                fscanf(stream.file, "%1023[^\"\\]", buf);

                char next = PeekTxt(stream.file);
                if (next == '\\') {
                    assert(fgetc(stream.file) == '\\');
                    next = fgetc(stream.file);
                    switch (next) {
                        case '"': {
                            str += buf;
                            str += '"';
                            continue;
                        }
                        case '\\': {
                            str += buf;
                            str += '\\';
                            continue;
                        }
                        case 'n': {
                            str += buf;
                            str += '\n';
                            continue;
                        }
                        default: {
                            ReportTxtError(stream.file, "unexpected escape sequence in string");
                        }
                    }
                    assert(!"unreachable");
                    exit(-1);
                }

                char end_quote = fgetc(stream.file);
                if (end_quote != '"') {
                    ReportTxtError(stream.file, "string too long, not supported");
                }
                str += buf;

                break;
            } while (1);
        }
    } else {
        PROC(strLen);
        str.resize(strLen);
        stream.process(str.data(), 1, strLen, stream.file);
    }
}
void Process(PackStream &stream, RNString &str)
{
    if (stream.mode == PACK_MODE_WRITE) {
        // HACK: avoid the string copy
        std::string &val = const_cast<std::string &>(str.str());
        PROC(val);
    } else {
        std::string val{};
        PROC(val);
        str = rnStringCatalog.Insert(val);
    }
}
template <typename T, size_t S>
void Process(PackStream &stream, std::array<T, S> &arr)
{
    assert(arr.size() < UINT16_MAX);
    uint16_t len = (uint16_t)arr.size();
    PROC(len);
    assert(len == S);
    for (int i = 0; i < len; i++) {
        PROC(arr[i]);
    }
}
template <typename T>
void Process(PackStream &stream, std::vector<T> &vec)
{
    assert(vec.size() < UINT16_MAX);
    uint16_t len = (uint16_t)vec.size();

    int wrap_width = 0;
    if (stream.type == PACK_TYPE_TEXT) {
        if constexpr (!(std::is_fundamental_v<T> || std::is_enum_v<T>)) {
            wrap_width = 1;
        }
        fprintf(stream.file, "\n");
    }

    PROC(len);
    vec.resize(len);

    if (wrap_width && len) {
        fprintf(stream.file, "\n");
    }

    for (int i = 0; i < len; i++) {
        PROC(vec[i]);
        if (wrap_width && i % wrap_width == 0 && i != len - 1) {
            fprintf(stream.file, "\n");
        }
    }
}
void Process(PackStream &stream, Vector2 &vec)
{
    PROC(vec.x);
    PROC(vec.y);
}
void Process(PackStream &stream, Vector3 &vec)
{
    PROC(vec.x);
    PROC(vec.y);
    PROC(vec.z);
}
void Process(PackStream &stream, Rectangle &rec)
{
    PROC(rec.x);
    PROC(rec.y);
    PROC(rec.width);
    PROC(rec.height);
}

void Process(PackStream &stream, GfxFile &gfx_file)
{
    HAQ_IO(HQT_GFX_FILE_FIELDS, gfx_file);
}
void Process(PackStream &stream, MusFile &mus_file)
{
    HAQ_IO(HQT_MUS_FILE_FIELDS, mus_file);
}
void Process(PackStream &stream, SfxFile &sfx_file)
{
    HAQ_IO(HQT_SFX_FILE_FIELDS, sfx_file);
}
void Process(PackStream &stream, GfxFrame &gfx_frame)
{
    HAQ_IO(HQT_GFX_FRAME_FIELDS, gfx_frame);

    if (stream.mode == PACK_MODE_READ) {
        stream.pack->gfx_frame_ids_by_gfx_file_name[gfx_frame.gfx].push_back(gfx_frame.id);
    }
}
void Process(PackStream &stream, GfxAnim &gfx_anim)
{
    HAQ_IO(HQT_GFX_ANIM_FIELDS, gfx_anim);
}
void Process(PackStream &stream, Sprite &sprite)
{
    HAQ_IO(HQT_SPRITE_FIELDS, sprite);
}
void Process(PackStream &stream, TileDef &tile_def)
{
    HAQ_IO(HQT_TILE_DEF_FIELDS, tile_def);
}
void Process(PackStream &stream, TileMat &tile_mat)
{
    HAQ_IO(HQT_TILE_MAT_FIELDS, tile_mat);
}
void Process(PackStream &stream, ObjectData &obj_data)
{
    HAQ_IO(HQT_OBJECT_DATA_FIELDS, obj_data);
}
void Process(PackStream &stream, AiPathNode &aiPathNode)
{
    PROC(aiPathNode.pos.x);
    PROC(aiPathNode.pos.y);
    PROC(aiPathNode.pos.z);
    PROC(aiPathNode.waitFor);
}
void Process(PackStream &stream, AiPath &aiPath)
{
    PROC(aiPath.pathNodeStart);
    PROC(aiPath.pathNodeCount);
}
void Process(PackStream &stream, Tilemap &tile_map)
{
    if (stream.mode == PackStreamMode::PACK_MODE_WRITE) {
        for (auto &layer : tile_map.layers) {
            assert(layer.size() == tile_map.width * tile_map.height);
        }
    }

    HAQ_IO(HQT_TILE_MAP_FIELDS, tile_map);
}
void Process(PackStream &stream, Entity &entity)
{
    bool alive = entity.id && !entity.despawned_at && entity.type;
    PROC(alive);
    if (!alive) {
        return;
    }

    //// Entity ////
    PROC(entity.id);
    PROC(entity.type);
    PROC(entity.spec);
    PROC(entity.name);
    PROC(entity.caused_by);
    PROC(entity.spawned_at);
    //PROC(entity.despawned_at);

    PROC(entity.map_id);
    PROC(entity.position);

    PROC(entity.ambient_fx);
    PROC(entity.ambient_fx_delay_min);
    PROC(entity.ambient_fx_delay_max);

    PROC(entity.radius);
    //PROC(entity.colliding);
    //PROC(entity.on_warp);

    //PROC(entity.last_attacked_at);
    //PROC(entity.attack_cooldown);

    PROC(entity.dialog_root_key);
    //PROC(entity.dialog_spawned_at);
    //PROC(entity.dialog_id);
    //PROC(entity.dialog_title);
    //PROC(entity.dialog_message);

    PROC(entity.hp_max);
    PROC(entity.hp);
    //PROC(entity.hp_smooth);

    PROC(entity.path_id);
    if (entity.path_id) {
        PROC(entity.path_node_last_reached);
        PROC(entity.path_node_target);
        //PROC(entity.path_node_arrived_at);
    }

    PROC(entity.drag);
    PROC(entity.speed);

    //PROC(entity.force_accum);
    //PROC(entity.velocity);

    PROC(entity.sprite_id);
    PROC(entity.direction);
    //PROC(entity.anim_frame);
    //PROC(entity.anim_accum);

    PROC(entity.warp_collider);
    PROC(entity.warp_dest_pos);

    PROC(entity.warp_dest_map);
    PROC(entity.warp_template_map);
    PROC(entity.warp_template_tileset);
}

template <typename T>
void AddToIndex(Pack &pack, T &dat, size_t index)
{
    const auto &by_id = pack.dat_by_id[T::dtype];
    pack.dat_by_id[T::dtype][dat.id] = index;

    const auto &by_name = pack.dat_by_name[T::dtype];
    pack.dat_by_name[T::dtype][dat.name] = index;
}

template <typename T>
void WriteArrayBin(PackStream &stream, std::vector<T> &vec)
{
    for (T &entry : vec) {
        stream.pack->toc.entries.push_back(PackTocEntry(entry.dtype, ftell(stream.file)));
        PROC(entry);
    }
}

template <typename T>
void ReadEntryBin(PackStream &stream, size_t index)
{
    auto &vec = *(std::vector<T> *)stream.pack->GetPool(T::dtype);
    auto &dat = vec[index];
    PROC(dat);

    if (stream.mode == PACK_MODE_READ) {
        AddToIndex(*stream.pack, dat, index);
    }
}

template <typename T>
void WriteArrayTxt(PackStream &stream, std::vector<T> &vec)
{
    for (T &entry : vec) {
        fprintf(stream.file, "%d", entry.dtype);
        PROC(entry);
        fprintf(stream.file, "\n");
    }
}

void IgnoreCommentsTxt(FILE *f)
{
    char next = fgetc(f);
    while (isspace(next) || next == '#') {
        if (next == '#') {
            while (next != '\n') {
                next = fgetc(f);
            }
        }
        next = fgetc(f);
    }
    fseek(f, ftell(f) - 1, SEEK_SET);
}

template <typename T>
void ReadEntryTxt(PackStream &stream)
{
    auto &vec = *(std::vector<T> *)stream.pack->GetPool(T::dtype);
    size_t index = vec.size();
    auto &dat = vec.emplace_back();
    PROC(dat);

    if (stream.mode == PACK_MODE_READ) {
        AddToIndex(*stream.pack, dat, index);
    }
}

Err Process(PackStream &stream)
{
    static const int MAGIC = 0x9291BBDB;
    // v1: the O.G. pack file
    // v2: add data buffers for gfx/mus/sfx
    // v3: add pitch_variance / multi to sfx
    // v4: add sprite resources
    static const int VERSION = 4;

    Err err = RN_SUCCESS;

    Pack &pack = *stream.pack;

    pack.magic = MAGIC;
    PROC(pack.magic);
    if (pack.magic != MAGIC) {
        return RN_BAD_FILE_READ;
    }

    pack.version = VERSION;
    PROC(pack.version);
    if (pack.version > VERSION) {
        return RN_BAD_FILE_READ;
    }

    int tocOffsetPos = ftell(stream.file);
    pack.toc_offset = 0;
    if (stream.type == PACK_TYPE_BINARY) {
        PROC(pack.toc_offset);
    }

    if (stream.mode == PACK_MODE_WRITE) {
        if (stream.type == PACK_TYPE_BINARY) {
            WriteArrayBin(stream, pack.gfx_files);
            WriteArrayBin(stream, pack.mus_files);
            WriteArrayBin(stream, pack.sfx_files);
            WriteArrayBin(stream, pack.gfx_frames);
            WriteArrayBin(stream, pack.gfx_anims);
            WriteArrayBin(stream, pack.sprites);
            WriteArrayBin(stream, pack.tile_defs);
            WriteArrayBin(stream, pack.tile_mats);
            WriteArrayBin(stream, pack.tile_maps);
            WriteArrayBin(stream, pack.entities);

            int tocOffset = ftell(stream.file);
            uint32_t entryCount = (uint32_t)pack.toc.entries.size();
            PROC(entryCount);
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
            }

            fseek(stream.file, tocOffsetPos, SEEK_SET);
            PROC(tocOffset);
        } else {
            fprintf(stream.file, "\n");

            #define HAQ_TXT_DOC_COMMENT_FIELD(c_type, c_name, c_init, flags, condition, userdata) \
                if constexpr ((flags) & HAQ_SERIALIZE) { \
                    if (condition) { \
                        fprintf(stream.file, " %s", #c_name); \
                    } \
                }

            #define HAQ_TXT_DOC_COMMENT(c_type_name, hqt_fields) \
                fprintf(stream.file, "\n# %s", #c_type_name); \
                hqt_fields(HAQ_TXT_DOC_COMMENT_FIELD, 0); \
                fprintf(stream.file, "\n");

            HAQ_TXT_DOC_COMMENT(GfxFile, HQT_GFX_FILE_FIELDS);
            WriteArrayTxt(stream, pack.gfx_files);
            HAQ_TXT_DOC_COMMENT(MusFile, HQT_MUS_FILE_FIELDS);
            WriteArrayTxt(stream, pack.mus_files);
            HAQ_TXT_DOC_COMMENT(SfxFile, HQT_SFX_FILE_FIELDS);
            WriteArrayTxt(stream, pack.sfx_files);
            HAQ_TXT_DOC_COMMENT(GfxFrame, HQT_GFX_FRAME_FIELDS);
            WriteArrayTxt(stream, pack.gfx_frames);
            HAQ_TXT_DOC_COMMENT(GfxAnim, HQT_GFX_ANIM_FIELDS);
            WriteArrayTxt(stream, pack.gfx_anims);
            HAQ_TXT_DOC_COMMENT(Sprite, HQT_SPRITE_FIELDS);
            WriteArrayTxt(stream, pack.sprites);
            HAQ_TXT_DOC_COMMENT(TileDef, HQT_TILE_DEF_FIELDS);
            WriteArrayTxt(stream, pack.tile_defs);
            HAQ_TXT_DOC_COMMENT(TileMat, HQT_TILE_MAT_FIELDS);
            WriteArrayTxt(stream, pack.tile_mats);
            HAQ_TXT_DOC_COMMENT(Tilemap, HQT_TILE_MAP_FIELDS);
            WriteArrayTxt(stream, pack.tile_maps);
            WriteArrayTxt(stream, pack.entities);

            #undef HAQ_TXT_DOC_COMMENT_TYPE
            #undef HAQ_TXT_DOC_COMMENT_FIELD
            #undef HAQ_TXT_DOC_COMMENT

            fprintf(stream.file, "\n# EOF marker\n%d", -1);
        }
    } else {
        if (stream.type == PACK_TYPE_BINARY) {
            fseek(stream.file, pack.toc_offset, SEEK_SET);
            uint32_t entryCount = 0;
            PROC(entryCount);
            pack.toc.entries.resize(entryCount);

            size_t typeCounts[DAT_TYP_COUNT]{};
            for (PackTocEntry &tocEntry : pack.toc.entries) {
                PROC(tocEntry.dtype);
                PROC(tocEntry.offset);
                typeCounts[tocEntry.dtype]++;
            }

            pack.gfx_files .resize(typeCounts[DAT_TYP_GFX_FILE]);
            pack.mus_files .resize(typeCounts[DAT_TYP_MUS_FILE]);
            pack.sfx_files .resize(typeCounts[DAT_TYP_SFX_FILE]);
            pack.gfx_frames.resize(typeCounts[DAT_TYP_GFX_FRAME]);
            pack.gfx_anims .resize(typeCounts[DAT_TYP_GFX_ANIM]);
            pack.sprites   .resize(typeCounts[DAT_TYP_SPRITE]);
            pack.tile_defs .resize(typeCounts[DAT_TYP_TILE_DEF]);
            pack.tile_mats .resize(typeCounts[DAT_TYP_TILE_MAT]);
            pack.tile_maps .resize(typeCounts[DAT_TYP_TILE_MAP]);
            pack.entities  .resize(typeCounts[DAT_TYP_ENTITY]);

            size_t typeNextIndex[DAT_TYP_COUNT]{};

            for (PackTocEntry &tocEntry : pack.toc.entries) {
                fseek(stream.file, tocEntry.offset, SEEK_SET);
                size_t index = typeNextIndex[tocEntry.dtype];
                tocEntry.index = index;
                switch (tocEntry.dtype) {
                    case DAT_TYP_GFX_FILE:  ReadEntryBin<GfxFile> (stream, index); break;
                    case DAT_TYP_MUS_FILE:  ReadEntryBin<MusFile> (stream, index); break;
                    case DAT_TYP_SFX_FILE:  ReadEntryBin<SfxFile> (stream, index); break;
                    case DAT_TYP_GFX_FRAME: ReadEntryBin<GfxFrame>(stream, index); break;
                    case DAT_TYP_GFX_ANIM:  ReadEntryBin<GfxAnim> (stream, index); break;
                    case DAT_TYP_SPRITE:    ReadEntryBin<Sprite>  (stream, index); break;
                    case DAT_TYP_TILE_DEF:  ReadEntryBin<TileDef> (stream, index); break;
                    case DAT_TYP_TILE_MAT:  ReadEntryBin<TileMat> (stream, index); break;
                    case DAT_TYP_TILE_MAP:  ReadEntryBin<Tilemap> (stream, index); break;
                    case DAT_TYP_ENTITY:    ReadEntryBin<Entity>  (stream, index); break;
                }
                typeNextIndex[tocEntry.dtype]++;
            }
        } else {
            while (!feof(stream.file)) {
                int dtype = DAT_TYP_INVALID;
                IgnoreCommentsTxt(stream.file);
                PROC(dtype);
                switch (dtype) {
                    case DAT_TYP_GFX_FILE:  ReadEntryTxt<GfxFile> (stream); break;
                    case DAT_TYP_MUS_FILE:  ReadEntryTxt<MusFile> (stream); break;
                    case DAT_TYP_SFX_FILE:  ReadEntryTxt<SfxFile> (stream); break;
                    case DAT_TYP_GFX_FRAME: ReadEntryTxt<GfxFrame>(stream); break;
                    case DAT_TYP_GFX_ANIM:  ReadEntryTxt<GfxAnim> (stream); break;
                    case DAT_TYP_SPRITE:    ReadEntryTxt<Sprite>  (stream); break;
                    case DAT_TYP_TILE_DEF:  ReadEntryTxt<TileDef> (stream); break;
                    case DAT_TYP_TILE_MAT:  ReadEntryTxt<TileMat> (stream); break;
                    case DAT_TYP_TILE_MAP:  ReadEntryTxt<Tilemap> (stream); break;
                    case DAT_TYP_ENTITY:    ReadEntryTxt<Entity>  (stream); break;
                    case -1: break;
                    default: ReportTxtError(stream.file, "Expected valid data type"); assert(!"asdf");
                }
            }
        }
    }

    return err;
}

#undef PROC
#undef HAQ_IO_FIELD
#undef HAQ_IO

Err SavePack(Pack &pack, PackStreamType type, std::string path)
{
    if (!path.size()) {
        path = pack.GetPath(type);
    }

    PerfTimer t{ TextFormat("Save pack %s", path.c_str()) };
    Err err = RN_SUCCESS;
#if 0
    if (FileExists(pack.path.c_str())) {
        err = MakeBackup(pack.path.c_str());
        if (err) return err;
    }
#endif
    FILE *file = fopen(path.c_str(), type == PACK_TYPE_BINARY ? "wb" : "w");
    if (!file) {
        return RN_BAD_FILE_WRITE;
    }

    PackStream stream{ &pack, file, PACK_MODE_WRITE, type };
    {
        PerfTimer t{ "Process" };
        err = Process(stream);
    }

    fclose(stream.file);

    if (err) {
        assert(!err);
        TraceLog(LOG_ERROR, "Failed to save pack file.\n");
    }
    return err;
}

void LoadSoundVariant(SfxFile &sfx_file, const char *path)
{
    SfxVariant sfx_variant{};
    sfx_variant.path = path;
    sfx_variant.sound = LoadSound(path);
    if (sfx_variant.sound.frameCount) {
        sfx_variant.instances.resize(sfx_file.max_instances);
        for (Sound &instance : sfx_variant.instances) {
            instance = LoadSoundAlias(sfx_variant.sound);
        }
    }
    sfx_file.variants.push_back(sfx_variant);
}
Err LoadPack(Pack &pack, PackStreamType type, std::string path)
{
    if (!path.size()) {
        path = pack.GetPath(type);
    }

    PerfTimer t{ TextFormat("Load pack %s", path.c_str()) };
    Err err = RN_SUCCESS;

    FILE *file = fopen(path.c_str(), type == PACK_TYPE_BINARY ? "rb" : "r");
    if (!file) {
        return RN_BAD_FILE_READ;
    }

    PackStream stream{ &pack, file, PACK_MODE_READ, type };

    {
        PerfTimer t{ "Process" };
        err = Process(stream);
    }

    if (stream.file) {
        fclose(stream.file);
    }

    if (err) {
        TraceLog(LOG_ERROR, "Failed to load data file.\n");
    }
    return err;
}
Err LoadResources(Pack &pack)
{
    Err err = RN_SUCCESS;

#if 0
    for (GfxFile &gfx_file : pack.gfx_files) {
        if (!gfx_file.data_buffer.length) {
            ReadFileIntoDataBuffer(gfx_file.path, gfx_file.data_buffer);
        }
        Image img = LoadImageFromMemory(".png", gfx_file.data_buffer.bytes, gfx_file.data_buffer.length);
        gfx_file.texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    for (MusFile &mus_file : pack.mus_files) {
        if (!mus_file.data_buffer.length) {
            ReadFileIntoDataBuffer(mus_file.path, mus_file.data_buffer);
        }
        mus_file.music = LoadMusicStreamFromMemory(".wav", mus_file.data_buffer.bytes, mus_file.data_buffer.length);
    }
    for (SfxFile &sfx_file : pack.sfx_files) {
        if (!sfx_file.data_buffer.length) {
            ReadFileIntoDataBuffer(sfx_file.path, sfx_file.data_buffer);
        }
        Wave wav = LoadWaveFromMemory(".wav", sfx_file.data_buffer.bytes, sfx_file.data_buffer.length);
        sfx_file.sound = LoadSoundFromWave(wav);
        UnloadWave(wav);
    }
#endif

    {
        PerfTimer t{ "Load graphics" };
        for (GfxFile &gfx_file : pack.gfx_files) {
            if (gfx_file.path.empty()) continue;
            gfx_file.texture = LoadTexture(gfx_file.path.c_str());
            SetTextureFilter(gfx_file.texture, TEXTURE_FILTER_POINT);
        }
    }
    {
        PerfTimer t{ "Load music" };
        for (MusFile &mus_file : pack.mus_files) {
            if (mus_file.path.empty()) continue;
            mus_file.music = LoadMusicStream(mus_file.path.c_str());
        }
    }
    {
        PerfTimer t{ "Load sounds" };
        for (SfxFile &sfx_file : pack.sfx_files) {
            if (sfx_file.path.empty()) continue;

            if (sfx_file.variations > 1) {
                const char *file_dir = GetDirectoryPath(sfx_file.path.c_str());
                const char *file_name = GetFileNameWithoutExt(sfx_file.path.c_str());
                const char *file_ext = GetFileExtension(sfx_file.path.c_str());
                for (int i = 1; i <= sfx_file.variations; i++) {
                    // Build variant path, e.g. chicken_cluck.wav -> chicken_cluck_01.wav
                    const char *variant_path = TextFormat("%s/%s_%02d%s", file_dir, file_name, i, file_ext);
                    LoadSoundVariant(sfx_file, variant_path);
                }
            } else {
                LoadSoundVariant(sfx_file, sfx_file.path.c_str());
            }
        }

#if _DEBUG
        // HACK(dlb): auto-generate tile ids for newly added tile defs
        uint16_t next_frame_id = 0;
        for (GfxFrame &gfx_frame : pack.gfx_frames) {
            if (gfx_frame.id < UINT16_MAX) {
                next_frame_id = MAX(next_frame_id, gfx_frame.id + 1);
            }
        }
        for (GfxFrame &gfx_frame : pack.gfx_frames) {
            if (gfx_frame.id == UINT16_MAX) {
                gfx_frame.id = next_frame_id++;
            }
            if (!gfx_frame.name.size()) {
                gfx_frame.name = TextFormat("frm_%s_%u_%u", gfx_frame.gfx.c_str(), gfx_frame.x, gfx_frame.y);
            }
        }
#endif
    }

    return err;
}
void UnloadPack(Pack &pack)
{
    for (GfxFile &gfxFile : pack.gfx_files) {
        UnloadTexture(gfxFile.texture);
    }
    for (MusFile &musFile : pack.mus_files) {
        UnloadMusicStream(musFile.music);
    }
    for (SfxFile &sfxFile : pack.sfx_files) {
        for (SfxVariant &sfx_variant : sfxFile.variants) {
            for (Sound &instance : sfx_variant.instances) {
                UnloadSoundAlias(instance);
            }
            UnloadSound(sfx_variant.sound);
        }
    }
}
