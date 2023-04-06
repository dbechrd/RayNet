#pragma once
#include "../common/common.h"
#include "../common/entity.h"

struct ServerPlayer {
    bool     needsClockSync {};
    uint32_t entityId       {};
    uint32_t lastInputSeq   {};  // sequence number of last input command we processed
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> inputQueue{};
};

struct ServerWorld {
    Entity entities[SV_MAX_ENTITIES]{};
    ServerPlayer players[yojimbo::MaxClients]{};
};
