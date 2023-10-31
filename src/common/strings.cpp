#include "strings.h"

StringCatalog rnStringCatalog;

void StringCatalog::Init(void)
{
    entries.resize(STR_COUNT);
    entries[STR_NULL] = "<null>";

    // TODO: LoadPack from ui.lang file
    entries[STR_UI_MENU_PLAY]     = "Play";
    entries[STR_UI_MENU_OPTIONS]  = "Options";
    entries[STR_UI_MENU_QUIT]     = "Quit";

    // TODO: LoadPack from dialog.lang file
    entries[STR_LILY_HELLO] = "Hello, welcome to the wonderful world of RayNet!";
}

const std::string &StringCatalog::GetString(StringId stringId)
{
    static const std::string nullstr = "<null>";
    if (stringId >= 0 && stringId < entries.size()) {
        return entries[stringId];
    }
    return nullstr;
}
#if 0
StringId StringCatalog::GetStringId(std::string value)
{
    const auto &entry = entriesByValue.find(value);
    if (entry != entriesByValue.end()) {
        return entry->second;
    }
    return STR_NULL;
}

void StringCatalog::SetString(StringId stringId, std::string value)
{
    std::string &existingStr = entries[stringId];
    if (existingStr.size()) {
        printf("[strings] Warning: String id %d exists with value %s, overwriting with %s.\n",
            stringId,
            existingStr.c_str(),
            value.c_str()
        );
        entries[stringId] = value;
        entriesByValue[value] = stringId;
        assert(!"Duplicate String");
    }
}
#endif
StringId StringCatalog::AddString(const std::string &value)
{
    if (!value.size()) {
        return STR_NULL;
    }

    const auto &entry = entriesByValue.find(value);
    if (entry == entriesByValue.end()) {
        size_t entryCount = entries.size();
        if (entryCount < UINT16_MAX) {
            StringId entryIdx = (uint16_t)entryCount;
            entries.push_back(value);
            entriesByValue[value] = entryIdx;
            return entryIdx;
        } else {
            TraceLog(LOG_ERROR, "ERROR: Cannot add string '%s' to string catalog. Catalog full.\n", value.c_str());
            return STR_NULL;
        }
    } else {
        return entry->second;
    }
}