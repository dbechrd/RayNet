#include "strings.h"

StringCatalog rnStringCatalog;

void StringCatalog::Init(void)
{
    entries.resize(STR_COUNT);
    entries[STR_NULL] = "<null>";

    entries[STR_SHT_CAMPFIRE] = "resources/campfire.txt";
    entries[STR_SHT_LILY] = "resources/lily.txt";

    entries[STR_SND_HARD_TICK] = "resources/soft_tick.wav";
    entries[STR_SND_SOFT_TICK] = "resources/soft_tick.wav";
    entries[STR_SND_MIC_TEST] = "resources/soft_tick.wav";

    // TODO: Load from ui.lang file
    entries[STR_UI_MENU_PLAY]     = "Play";
    entries[STR_UI_MENU_OPTIONS]  = "Options";
    entries[STR_UI_MENU_QUIT]     = "Quit";

    // TODO: Load from dialog.lang file
    entries[STR_LILY_HELLO] = "Hello, welcome to the wonderful world of RayNet!";
}

std::string StringCatalog::GetString(StringId stringId)
{
    if (stringId >= 0 && stringId < entries.size()) {
        return entries[stringId];
    }
    return "<null>";
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
StringId StringCatalog::AddString(std::string value)
{
    if (!value.size()) {
        return STR_NULL;
    }

    const auto &entry = entriesByValue.find(value);
    if (entry == entriesByValue.end()) {
        StringId entryIdx = entries.size();
        entries.push_back(value);
        entriesByValue[value] = entryIdx;
        return entryIdx;
    } else {
        return entry->second;
    }
}