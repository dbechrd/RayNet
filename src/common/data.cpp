#include "data.h"
#include "file_utils.h"
#include "net/net.h"
#include "perf_timer.h"

ENUM_STR_CONVERTER(DataTypeStr, DataType, DATA_TYPES, ENUM_VD_CASE_RETURN_DESC);
ENUM_STR_CONVERTER(ObjTypeStr, ObjType, OBJ_TYPES, ENUM_VD_CASE_RETURN_DESC);
ENUM_META_CONVERTER(ObjTypeTileDefId, ObjType, OBJ_TYPES, ENUM_VDM_CASE_RETURN_META, uint16_t, 0);

struct GameState {
    bool freye_introduced;
};
GameState game_state;

Pack pack_assets{ "assets" };
Pack pack_maps{ "maps" };

Shader shdSdfText;
Shader shdPixelFixer;
int    shdPixelFixerScreenSizeUniformLoc;

Font fntTiny;
Font fntSmall;
Font fntMedium;
Font fntBig;

uint32_t FreyeIntroListener(uint32_t source_id, uint32_t target_id, uint32_t dialog_id) {
    uint32_t final_dialog_id = dialog_id;
    if (!game_state.freye_introduced) {
        // Freye not yet introduced, redirect player to full introduction
        Dialog *first_time_intro = dialog_library.FindByKey("DIALOG_FREYE_INTRO_FIRST_TIME");
        if (first_time_intro) {
            final_dialog_id = first_time_intro->id;
        } else {
            assert(!"probably this shouldn't fail bro, fix ur dialogs");
        }
        game_state.freye_introduced = true;
    }
    return final_dialog_id;
}

// TODO(dlb)[cleanup]: Data buffer stuff
#if 0
void ReadFileIntoDataBuffer(const std::string &filename, DatBuffer &datBuffer)
{
    int bytes_read = 0;
    datBuffer.bytes = LoadFileData(filename.c_str(), &bytes_read);
    datBuffer.length = bytes_read;
}
void FreeDataBuffer(DatBuffer &datBuffer)
{
    if (datBuffer.length) {
        MemFree(datBuffer.bytes);
        datBuffer = {};
    }
}

void Process(PackStream &stream, DatBuffer &buffer)
{
    PROC(buffer.length);
    if (buffer.length) {
        if (!buffer.bytes) {
            assert(stream.mode == PACK_MODE_READ);
            buffer.bytes = (uint8_t *)MemAlloc(buffer.length);
        }
        for (size_t i = 0; i < buffer.length; i++) {
            PROC(buffer.bytes[i]);
        }
    }
}
#endif

void CompressFile(const char *srcFileName, const char *dstFileName)
{
    int rawSize = 0;
    uint8_t *rawBytes = LoadFileData(srcFileName, &rawSize);

    int compressedSize = 0;
    uint8_t *compressedBytes = CompressData(rawBytes, rawSize, &compressedSize);

    SaveFileData(dstFileName, compressedBytes, compressedSize);

    MemFree(rawBytes);
    MemFree(compressedBytes);
}

void GenTileDefs(const std::string &gfx, int tiles_w, int tiles_h, int tile_w, int tile_h)
{
    for (int y = 0; y < tiles_h; y++) {
        for (int x = 0; x < tiles_w; x++) {
            printf("3 %u \"\" \"%s\" %u %u %u %u\n", UINT16_MAX, gfx.c_str(), x * tile_w, y * tile_h, tile_w, tile_h);
        }
    }
}

Err Init(void)
{
#if 0
    GenTileDefs("gfx_til_cave", 16, 16, 64, 64);
#endif

#if 0
    uint8_t seq_c = 253;
    uint8_t seq_s = 243;

    printf("s   c   c > s\n");
    for (int i = 0; i < 20; i++) {
        bool ahead = paws_greater(seq_c, seq_s);
        assert(ahead);
        printf("%-3u %-3u %s\n", seq_s, seq_c, ahead ? "true" : "false");
        seq_c++;
        seq_s++;
    }

    seq_c = 243;
    seq_s = 253;

    printf("s   c   c > s\n");
    for (int i = 0; i < 20; i++) {
        bool ahead = paws_greater(seq_c, seq_s);
        assert(!ahead);
        printf("%-3u %-3u %s\n", seq_s, seq_c, ahead ? "true" : "false");
        seq_c++;
        seq_s++;
    }

    printf("");
#endif

#if 0
    {
        uint8_t data[]{ 1, 2, 3, 4, 5 };

        int deflated_bytes = 0;
        int inflated_bytes = 0;

        uint8_t *deflated = 0;
        uint8_t *inflated = 0;

        // 1 - 4 bytes works fine
        deflated = CompressData(data, 1, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 2, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 3, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 4, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);

        // 5 - 8 bytes asserts during DecompressData()
        deflated = CompressData(data, 5, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 6, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 7, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
        deflated = CompressData(data, 8, &deflated_bytes);
        inflated = DecompressData(deflated, deflated_bytes, &inflated_bytes);
    }
#endif

#if HAQ_ENABLE_SCHEMA
    haq_print(hqt_gfx_file_schema);
    printf("");
#endif

#if 0
    NewEnt::run_tests();
#endif

    PerfTimer t{ "InitCommon" };

    Err err = RN_SUCCESS;

    rnStringCatalog.Init();

    // Load SDF required shader (we use default vertex shader)
    shdSdfText = LoadShader(0, "resources/shader/sdf.fs");

    shdPixelFixer                     = LoadShader("resources/shader/pixelfixer.vs", "resources/shader/pixelfixer.fs");
    shdPixelFixerScreenSizeUniformLoc = GetShaderLocation(shdPixelFixer, "screenSize");

#if 0
    const char *fontName = "C:/Windows/Fonts/consolab.ttf";
    if (!FileExists(fontName)) {
        fontName = "resources/font/KarminaBold.otf";
    }
#else
    //const char *fontName = "resources/font/KarminaBold.otf";
    const char *fontName = "resources/font/Inconsolata-SemiBold.ttf";
    //const char *fontName = "resources/font/PixelOperator-Bold.ttf";
    //const char *fontName = "resources/font/PixelOperatorMono-Bold.ttf";
    //const char *fontName = "resources/font/FiraMono-Medium.ttf";
#endif

    fntTiny = dlb_LoadFontEx(fontName, 14, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntTiny.baseSize, RN_RAYLIB_ERROR);

    fntSmall = dlb_LoadFontEx(fontName, 16, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntSmall.baseSize, RN_RAYLIB_ERROR);

    fntMedium = dlb_LoadFontEx(fontName, 20, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntMedium.baseSize, RN_RAYLIB_ERROR);

    fntBig = dlb_LoadFontEx(fontName, 46, 0, 0, FONT_DEFAULT);
    ERR_RETURN_EX(fntBig.baseSize, RN_RAYLIB_ERROR);
    //SetTextureFilter(fntBig.texture, TEXTURE_FILTER_BILINEAR);    // Required for SDF font

    //GenPlaceholderTexture();

#if DEV_BUILD_PACK_FILE
    {
        PerfTimer t{ "Build pack files" };

        Pack buildAssets{ "assets" };
        ERR_RETURN(LoadPack(buildAssets, PACK_TYPE_TEXT));
        ERR_RETURN(SavePack(buildAssets, PACK_TYPE_BINARY));
#if SV_SERVER
        Pack buildMaps{ "maps" };
        ERR_RETURN(LoadPack(buildMaps, PACK_TYPE_TEXT));
        ERR_RETURN(SavePack(buildMaps, PACK_TYPE_BINARY));
#endif
    }
#endif

    ERR_RETURN(LoadPack(pack_assets, PACK_TYPE_BINARY));
    ERR_RETURN(LoadResources(pack_assets));
#if SV_SERVER
    ERR_RETURN(LoadPack(pack_maps, PACK_TYPE_BINARY));
    ERR_RETURN(LoadResources(pack_maps));
#endif

#if 0
    // TODO: make dlb_CompressFile and Use LZ4 instead if we want to do this.
    CompressFile("pack/assets.dat", "pack/assets.smol");
#endif

    // TODO: Pack these too
    LoadDialogFile("resources/dialog/lily.md");
    LoadDialogFile("resources/dialog/freye.md");
    LoadDialogFile("resources/dialog/nessa.md");
    LoadDialogFile("resources/dialog/elane.md");

    dialog_library.RegisterListener("DIALOG_FREYE_INTRO", FreyeIntroListener);

    return err;
}
void Free(void)
{
    UnloadShader(shdSdfText);
    UnloadShader(shdPixelFixer);
    UnloadFont(fntTiny);
    UnloadFont(fntSmall);
    UnloadFont(fntMedium);
    UnloadFont(fntBig);

    // NOTE(dlb): ~Material is crashing for unknown reason. double free or trying to free string constant??
    // NOTE(dlb): delete pack takes *forever*. Who cares. Let OS figure it out.
#ifndef FAST_EXIT_SKIP_FREE
    for (auto pack : packs) {
        UnloadPack(*pack);
        delete pack;
    }
    packs.clear();

    for (auto &fileText : dialog_library.dialog_files) {
        UnloadFileText((char *)fileText);
    }
#endif
}

const SfxVariant &PickSoundVariant(const SfxFile &sfx_file)
{
    auto &variants = sfx_file.variants;
    if (variants.size() > 1) {
        const size_t variant_idx = (size_t)GetRandomValue(0, variants.size() - 1);
        return variants[variant_idx];
    } else {
        return variants[0];
    }
}

void PlaySound(const std::string &id, float pitchVariance)
{
    const SfxFile &sfx_file = pack_assets.FindByName<SfxFile>(id);
    const SfxVariant &sfx_variant = PickSoundVariant(sfx_file);
    assert(sfx_variant.instances.size() == sfx_file.max_instances);

    //printf("updatesprite playsound %s (multi = %d)\n", sfx_file.path.c_str(), sfx_file.multi);
    for (const Sound &instance : sfx_variant.instances) {
        if (!IsSoundPlaying(instance)) {
            float variance = pitchVariance ? pitchVariance : sfx_file.pitch_variance;
            SetSoundPitch(instance, 1.0f + GetRandomFloatVariance(variance));
            PlaySound(instance);
            break;
        }
    }
}
bool IsSoundPlaying(const std::string &id)
{
    const SfxFile &sfx_file = pack_assets.FindByName<SfxFile>(id);
    const SfxVariant &sfx_variant = PickSoundVariant(sfx_file);
    assert(sfx_variant.instances.size() == sfx_file.max_instances);

    bool playing = false;
    for (const Sound &instance : sfx_variant.instances) {
        playing = IsSoundPlaying(instance);
        if (playing) break;
    }
    return playing;
}
void StopSound(const std::string &id)
{
    const SfxFile &sfx_file = pack_assets.FindByName<SfxFile>(id);
    const SfxVariant &sfx_variant = PickSoundVariant(sfx_file);
    assert(sfx_variant.instances.size() == sfx_file.max_instances);

    for (const Sound &instance : sfx_variant.instances) {
        StopSound(instance);
    }
}

void UpdateGfxAnim(const GfxAnim &anim, double dt, GfxAnimState &anim_state)
{
    anim_state.accum += dt;

    const double anim_frame_dt = 1.0 / 60.0;
    const double frame_time = anim.frame_delay * anim_frame_dt;
    if (anim_state.accum >= frame_time) {
        anim_state.frame++;
        if (anim_state.frame >= anim.frames.size()) {
            anim_state.frame = 0;
        }
        anim_state.accum -= frame_time;
    }
}

void UpdateSprite(Entity &entity, double dt, bool newlySpawned)
{
    // TODO: Make this more general and stop taking in entityType.
    switch (entity.type) {
        case Entity::TYP_PLAYER: case Entity::TYP_NPC: {
            if (entity.velocity.x > 0) {
                entity.direction = DIR_E;
            } else if (entity.velocity.x < 0) {
                entity.direction = DIR_W;
            } else if (!entity.direction) {
                entity.direction = DIR_E;  // HACK: we don't have north sprites atm
            }
            break;
        }
    }

    const Sprite &sprite = pack_assets.FindById<Sprite>(entity.sprite_id);
    const GfxAnim &anim = pack_assets.FindByName<GfxAnim>(sprite.anims[entity.direction]);
    UpdateGfxAnim(anim, dt, entity.anim_state);

    if (newlySpawned && anim.sound != "null") {
        const SfxFile &sfx_file = pack_assets.FindByName<SfxFile>(anim.sound);
        PlaySound(sfx_file.name);  // TODO: Play by id instead
    }
}
void ResetSprite(Entity &entity)
{
    const Sprite &sprite = pack_assets.FindById<Sprite>(entity.sprite_id);
    GfxAnim &anim = pack_assets.FindByName<GfxAnim>(sprite.anims[entity.direction]);
    StopSound(anim.sound);

    entity.anim_state.frame = 0;
    entity.anim_state.accum = 0;
}
void DrawSprite(const Entity &entity, DrawCmdQueue *sortedDraws, bool highlight)
{
    const GfxFrame &frame = entity.GetSpriteFrame();
    const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(frame.gfx);

    Rectangle sprite_rect = entity.GetSpriteRect();
    Vector3 pos = { sprite_rect.x, sprite_rect.y };
    pos.x = floorf(pos.x);
    pos.y = floorf(pos.y);
    //if (Vector3LengthSqr(entity.velocity) < 0.0001f) {
    //    pos.x = floorf(pos.x);
    //    pos.y = floorf(pos.y);
    //}

    Color color = WHITE;
    if (entity.on_warp_cooldown) {
        color = ColorTint(color, SKYBLUE);
    }
    if (highlight) {
        color = ColorBrightness(color, 1.3f);
    }

    const Rectangle frame_rec{ (float)frame.x, (float)frame.y, (float)frame.w, (float)frame.h };
    if (sortedDraws) {
        DrawCmd cmd = DrawCmd::Texture(gfx_file.texture, frame_rec, pos, color);
        sortedDraws->push(cmd);
    } else {
        const Vector2 sprite_pos{ pos.x, pos.y - pos.z };
        dlb_DrawTextureRec(gfx_file.texture, frame_rec, sprite_pos, color);
    }
}

void UpdateTileDefAnimations(double dt)
{
    for (TileDef &tile_def : pack_assets.tile_defs) {
        const GfxAnim &anim = pack_assets.FindByName<GfxAnim>(tile_def.anim);
        UpdateGfxAnim(anim, dt, tile_def.anim_state);
    }
}

TileDef &GetTileDef(uint16_t tile_id)
{
    if (tile_id < pack_assets.tile_defs.size()) {
        return pack_assets.tile_defs[tile_id];
    }
    return pack_assets.tile_defs[0];
}
const GfxFrame &GetTileGfxFrame(uint16_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    const GfxAnim &gfx_anim = pack_assets.FindByName<GfxAnim>(tile_def.anim);
    const std::string &gfx_frame_id = gfx_anim.GetFrame(tile_def.anim_state.frame);
    const GfxFrame &gfx_frame = pack_assets.FindByName<GfxFrame>(gfx_frame_id);
    return gfx_frame;
}
Rectangle TileDefRect(uint16_t tile_id)
{
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const Rectangle rect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    return rect;
}
Color TileDefAvgColor(uint16_t tile_id)
{
    const TileDef &tile_def = GetTileDef(tile_id);
    return tile_def.color;
}
void DrawTile(uint16_t tile_id, Vector2 position, DrawCmdQueue *sortedDraws, Color color)
{
    // TODO: Yikes.. that's a lot of lookups in a tight loop. Memoize some pointers or something man.
    const GfxFrame &gfx_frame = GetTileGfxFrame(tile_id);
    const GfxFile &gfx_file = pack_assets.FindByName<GfxFile>(gfx_frame.gfx);
    Rectangle texRect{ (float)gfx_frame.x, (float)gfx_frame.y, (float)gfx_frame.w, (float)gfx_frame.h };
    if (gfx_frame.h > TILE_W) {
        position.y -= gfx_frame.h - TILE_W;
    }

    if (sortedDraws) {
        Vector3 pos = { position.x, position.y, 0 };
        DrawCmd cmd = DrawCmd::Texture(gfx_file.texture, texRect, pos, color);
        sortedDraws->push(cmd);
    } else {
        dlb_DrawTextureRec(gfx_file.texture, texRect, position, color);
    }
}