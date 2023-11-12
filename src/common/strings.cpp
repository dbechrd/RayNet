#include "strings.h"

const std::string &StringCatalog::strNull = "<null>";

const RNString &rnStringNull = { STR_NULL };
StringCatalog rnStringCatalog;

const std::string &RNString::str(void)
{
    return rnStringCatalog.Find(*this);
}

void StringCatalog::Init(void)
{
    entries.resize(STR_COUNT);
    entries[STR_NULL] = strNull;

    // TODO: LoadPack from ui.lang file
    entries[STR_UI_MENU_PLAY]     = "Play";
    entries[STR_UI_MENU_OPTIONS]  = "Options";
    entries[STR_UI_MENU_QUIT]     = "Quit";

    // TODO: LoadPack from dialog.lang file
    entries[STR_LILY_HELLO] = "Hello, welcome to the wonderful world of RayNet!";
}

const std::string &StringCatalog::Find(RNString str)
{
    if (str.id >= 0 && str.id < entries.size()) {
        return entries[str.id];
    }
    return strNull;
}
const RNString StringCatalog::FindByValue(const std::string &value)
{
    const auto &entry = entriesByValue.find(value);
    if (entry != entriesByValue.end()) {
        return entry->second;
    }
    return rnStringNull;
}
const RNString StringCatalog::Insert(const std::string &value)
{
    // TODO(dlb): Should interning "" be allowed??
    if (!value.size()) {
        return rnStringNull;
    }

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
            return rnStringNull;
        }
    } else {
        return entry->second;
    }
}