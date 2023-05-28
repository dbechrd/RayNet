#pragma once
#include "../../common.h"

struct Msg_S_EntitySnapshot : public yojimbo::Message
{
    double      serverTime {};

    // Entity
    uint32_t    mapId      {};
    uint32_t    entityId   {};
    EntityType  type       {};  // doesn't change, but needed for switch statements in deserializer
    Vector2     position   {};

    // Collision
    //float       radius     {};  // when would this ever change? doesn't.. for now.

    // Physics
    float       speed      {};
    Vector2     velocity   {};

    // Life
    int         maxHealth  {};
    int         health     {};

    // Only for Entity_Player
    // TODO: Only send this to the player who actually owns this player entity,
    //       otherwise we're leaking info about other players' connections.
    int         clientIdx  {};  // TODO: Populate this for lastprocessinputcmd
    uint32_t    lastProcessedInputCmd {};

    template <typename Stream> bool Serialize(Stream &stream)
    {
        serialize_double(stream, serverTime);

        // Entity
        serialize_uint32(stream, mapId);
        serialize_uint32(stream, entityId);
        serialize_uint32(stream, (uint32_t &)type);
        serialize_float(stream, position.x);
        serialize_float(stream, position.y);

        // Physics
        switch (type) {
            case Entity_NPC:
            case Entity_Player:
            case Entity_Projectile:
            {
                serialize_float(stream, velocity.x);
                serialize_float(stream, velocity.y);
            }
        }

        // Life
        switch (type) {
            case Entity_NPC:
            case Entity_Player:
            {
                serialize_varint32(stream, maxHealth);
                serialize_varint32(stream, health);
                break;
            }
        }

        // TODO: Also check if player is the clientIdx player. I.e. don't leak
        //       input info to all other players.
        if (type == Entity_Player) {
            serialize_uint32(stream, lastProcessedInputCmd);
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};