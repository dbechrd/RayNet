#pragma once
#include "input_command.h"
#include "net/entity_state.h"
#include "ring_buffer.h"

struct Entity {
    Color color;
    Vector2 size;

    float speed;
    Vector2 velocity;
    Vector2 position;

    void Serialize(EntityState &entityState);
    void ApplyStateInterpolated(const EntityState &a, const EntityState &b, double alpha);
    void Draw(const Font &font, int clientIdx);
};

struct ServerPlayer {
    bool     needsClockSync {};
    Entity   entity         {};  // TODO(dlb): entityIdx into an actual world state
    uint32_t lastInputSeq   {};  // sequence number of last input command we processed
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> inputQueue{};
};

struct ClientEntity {
    Entity entity{};
    RingBuffer<EntityState, CL_SNAPSHOT_COUNT> snapshots{};
};