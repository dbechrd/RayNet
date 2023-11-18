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
    const std::string &Find(RNString str);                 // get string data, should be used via RNString.str()
    const RNString FindByValue(const std::string &value);  // existence check; find without inserting
    const RNString Insert(const std::string &value);       // find or insert

private:
    static const std::string &strNull;

    std::vector<std::string> entries;
    std::unordered_map<std::string, RNString> entriesByValue{};

    // TODO(dlb): How to handle dynamic strings.. e.g. when you open a new tileset
    // image in the editor and want to change tilemap.tileset to it?
};

extern const RNString &rnStringNull;
extern StringCatalog rnStringCatalog;
