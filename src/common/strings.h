#pragma once
#include "common.h"

typedef size_t StringId;

enum {
    STR_NULL,

    // Resource IDs (internal, not for users)
    STR_SHT_CAMPFIRE,
    STR_SHT_LILY,

    STR_SND_HARD_TICK,
    STR_SND_MIC_TEST,
    STR_SND_SOFT_TICK,

    // UI Text
    STR_UI_MENU_OPTIONS,
    STR_UI_MENU_PLAY,
    STR_UI_MENU_QUIT,

    // Dialog
    STR_LILY_HELLO,

    STR_COUNT
};

struct StringCatalog {
    void Init(void);
    std::string GetString(StringId stringId);
    //StringId GetStringId(std::string value);
    StringId AddString(std::string value);

private:
    //void SetString(StringId stringId, std::string value);

    std::vector<std::string> entries;
    std::unordered_map<std::string, size_t> entriesByValue{};

    // TODO(dlb): How to handle dynamic strings.. e.g. when you open a new tileset
    // image in the editor and want to change tilemap.tileset to it?
};

extern StringCatalog rnStringCatalog;