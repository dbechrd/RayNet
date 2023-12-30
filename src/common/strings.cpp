#include "strings.h"

StringCatalog rnStringCatalog;

// Data
RNString S_NULL;
RNString S_LEVER;
RNString S_DOOR;
RNString S_LOOTABLE;
RNString S_SIGN;
RNString S_WARP;

// UI Text
RNString SLOCAL_UI_MENU_OPTIONS;
RNString SLOCAL_UI_MENU_PLAY;
RNString SLOCAL_UI_MENU_QUIT;

// Dialog
RNString SLOCAL_LILY_HELLO;

const std::string &RNString::str(void) const
{
    return rnStringCatalog.Find(*this);
}

void StringCatalog::Init(void)
{
    S_NULL     = Insert("<null>");
    S_LEVER    = Insert("lever");
    S_DOOR     = Insert("door");
    S_LOOTABLE = Insert("lootable");
    S_SIGN     = Insert("sign");
    S_WARP     = Insert("warp");

    // TODO: LoadPack from ui.lang file
    SLOCAL_UI_MENU_PLAY    = Insert("Play");
    SLOCAL_UI_MENU_OPTIONS = Insert("Options");
    SLOCAL_UI_MENU_QUIT    = Insert("Quit");

    // TODO: LoadPack from dialog.lang file
    SLOCAL_LILY_HELLO = Insert("Hello, welcome to the wonderful world of RayNet!");
}

const std::string &StringCatalog::Find(RNString str)
{
    if (str.id >= 0 && str.id < entries.size()) {
        return entries[str.id];
    }
    return entries[S_NULL.id];
}
const RNString StringCatalog::FindByValue(const std::string &value)
{
    const auto &entry = entriesByValue.find(value);
    if (entry != entriesByValue.end()) {
        return entry->second;
    }
    return S_NULL;
}
const RNString StringCatalog::Insert(const std::string &value)
{
    const auto &entry = entriesByValue.find(value);
    if (entry == entriesByValue.end()) {
        size_t entryCount = entries.size();
        if (entryCount < UINT16_MAX) {
            const RNString rnString{ (uint16_t)entryCount };
            entries.push_back(value);
            entriesByValue[value] = rnString;
            return rnString;
        } else {
            TraceLog(LOG_ERROR, "ERROR: Cannot add string '%s' to string catalog. Catalog full.\n", value.c_str());
            return S_NULL;
        }
    } else {
        return entry->second;
    }
}