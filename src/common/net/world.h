#pragma once
#include "../common.h"
#include <queue>

template <typename T_Player>
struct World {
    T_Player players[yojimbo::MaxClients]{};
};
