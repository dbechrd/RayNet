#pragma once
#include "../common/common.h"
#include "../common/input_command.h"
#include "../common/net/net.h"
#include "../common/ring_buffer.h"
#include "todo.h"

struct ClientWorld;

struct Controller {
    int nextSeq{};  // next input command sequence number to use

    InputCmd cmdAccum{};         // accumulate input until we're ready to sample
    double sampleInputAccum{};   // when this fills up, we are due to sample again
    double lastInputSampleAt{};  // time we last sampled accumulator
    double lastCommandSentAt{};  // time we last sent inputs to the server
    RingBuffer<InputCmd, CL_SEND_INPUT_COUNT> cmdQueue{};  // queue of last N input samples
};

struct GameClient {
    NetAdapter adapter;
    yojimbo::Client *yj_client{};
    double clientTimeDeltaVsServer{};  // how far ahead/behind client clock is
    double netTickAccum{};             // when this fills up, a net tick is due
    double lastNetTick{};              // for fixed-step networking updates
    double now{};                      // current time for this frame

    Controller controller{};
    uint32_t hoveredEntityId{};
    ClientWorld *world{};
    TodoList todoList{};

    inline double ServerNow(void) {
        const double serverNow = now - clientTimeDeltaVsServer;
        return serverNow;
    }

    uint32_t LocalPlayerEntityId(void);
    Entity *LocalPlayer(void);
    Err TryConnect(void);
    void Start(void);
    void SendInput(const Controller &controller);
    void SendEntityInteract(uint32_t entityId);
    void SendTileInteract(uint32_t x, uint32_t y);
    void ProcessMessages(void);
    void Update(void);
    void Stop(void);
};
