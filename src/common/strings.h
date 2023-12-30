#pragma once
#include "common.h"

struct RNString {
    uint16_t id{};

    const std::string &str(void) const;

    bool operator==(const RNString &other) const
    {
        return id == other.id;
    }

    struct Hasher {
        const std::hash<uint16_t> hasher{};

        std::size_t operator()(const RNString &str) const
        {
            return hasher(str.id);
        }
    };
};

struct StringCatalog {
    void Init(void);
    const std::string &Find(RNString str);                 // get string data, should be used via RNString.str()
    const RNString FindByValue(const std::string &value);  // existence check; find without inserting
    const RNString Insert(const std::string &value);       // find or insert

private:

    std::vector<std::string> entries;
    std::unordered_map<std::string, RNString> entriesByValue{};

    // TODO(dlb): How to handle dynamic strings.. e.g. when you open a new tileset
    // image in the editor and want to change tilemap.tileset to it?
};

extern StringCatalog rnStringCatalog;

// S_*      = string literal w/ internal use only (e.g. pack files)
// SLOCAL_* = in-game string that should be loaded dynamically w/ localization

// Data
extern RNString S_NULL;
extern RNString S_LEVER;
extern RNString S_DOOR;
extern RNString S_LOOTABLE;
extern RNString S_SIGN;
extern RNString S_WARP;

// UI Text
extern RNString SLOCAL_UI_MENU_OPTIONS;
extern RNString SLOCAL_UI_MENU_PLAY;
extern RNString SLOCAL_UI_MENU_QUIT;

// Dialog
extern RNString SLOCAL_LILY_HELLO;

