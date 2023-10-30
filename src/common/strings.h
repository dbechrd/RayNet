#pragma once
#include "common.h"

typedef uint16_t StringId;

enum {
    STR_NULL,

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
    const std::string &GetString(StringId stringId);
    //StringId GetStringId(std::string value);
    StringId AddString(const std::string &value);

private:
    //void SetString(StringId stringId, std::string value);

    std::vector<std::string> entries;
    std::unordered_map<std::string, StringId> entriesByValue{};

    // TODO(dlb): How to handle dynamic strings.. e.g. when you open a new tileset
    // image in the editor and want to change tilemap.tileset to it?
};

extern StringCatalog rnStringCatalog;