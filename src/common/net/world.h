#pragma once
#include "../common.h"
#include "../player.h"

struct World {
    Player players[yojimbo::MaxClients];
};
extern World *g_world;
