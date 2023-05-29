#include "net.h"

const char *MsgTypeStr(MsgType type)
{
    switch (type) {
        case MSG_C_ENTITY_INTERACT: return "MSG_C_ENTITY_INTERACT";
        case MSG_C_INPUT_COMMANDS:  return "MSG_C_INPUT_COMMANDS";
        case MSG_C_TILE_INTERACT:   return "MSG_C_TILE_INTERACT";

        case MSG_S_CLOCK_SYNC:      return "MSG_S_CLOCK_SYNC";
        case MSG_S_ENTITY_DESPAWN:  return "MSG_S_ENTITY_DESPAWN";
        case MSG_S_ENTITY_SAY:      return "MSG_S_ENTITY_SAY";
        case MSG_S_ENTITY_SNAPSHOT: return "MSG_S_ENTITY_SNAPSHOT";
        case MSG_S_ENTITY_SPAWN:    return "MSG_S_ENTITY_SPAWN";
        case MSG_S_TILE_CHUNK:      return "MSG_S_TILE_CHUNK";

        default:                    return "<UNKNOWN_MSG_TYPE>";
    }
}

void InitClientServerConfig(yojimbo::ClientServerConfig &config)
{
    config.numChannels = CHANNEL_COUNT;
    config.channel[CHANNEL_U_INPUT_COMMANDS].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_U_ENTITY_SNAPSHOT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    config.channel[CHANNEL_R_CLOCK_SYNC].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[CHANNEL_R_ENTITY_EVENT].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[CHANNEL_R_TILE_EVENT].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;

    config.bandwidthSmoothingFactor = CL_BANDWIDTH_SMOOTHING_FACTOR;
}

int yj_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[yojimbo] ");
    int result = vprintf(format, args);
    va_end(args);
    return result;
}