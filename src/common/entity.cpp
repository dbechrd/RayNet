#include "entity.h"

const char *EntityTypeStr(EntityType type)
{
    switch (type) {
        case Entity_None:       return "Entity_None";
        case Entity_Player:     return "Entity_Player";
        case Entity_NPC:        return "Entity_NPC";
        case Entity_Projectile: return "Entity_Projectile";
        default:                return "<UNKNOWN_ENTITY_TYPE>";
    }
}