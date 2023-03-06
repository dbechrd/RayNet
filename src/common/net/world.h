#pragma once
#include "../common.h"

template <typename T_Player>
struct World {
    T_Player players[yojimbo::MaxClients]{};
};
